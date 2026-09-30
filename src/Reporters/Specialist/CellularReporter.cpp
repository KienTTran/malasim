/*
 * CellularReporter.cpp
 *
 * Implementation of the CellularReporter class.
 */

#include <cmath>
#include "CellularReporter.h"
#include "MDC/ModelDataCollector.h"
#include "Simulation/Model.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Population.h"
#include "Parasites/Genotype.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

void CellularReporter::initialize(int job_number, const std::string &path) {
  // Check the number of cells in the model, if it exceeds one, throw an error
  if (Model::get_config()->location_db().size() > 1) {
    throw std::runtime_error(
        "CellularReporter can only be used with one location configuration.");
  }

  // Configure file-based loggers using spdlog
  std::string aggregate_log_file = fmt::format("{}_aggregate_data_{}.csv", path, job_number);
  std::string detail_log_file = fmt::format("{}_detailed_data_{}.csv", path, job_number);
  std::string blood_log_file = fmt::format("{}_blood_data_{}.csv", path, job_number);

  auto aggregate_logger = spdlog::basic_logger_mt("aggregate_reporter", aggregate_log_file);
  auto detail_logger = spdlog::basic_logger_mt("detail_reporter", detail_log_file);
  auto blood_logger = spdlog::basic_logger_mt("blood_reporter", blood_log_file);

//  spdlog::info("Initialized loggers: {}, {}, {}", aggregate_log_file, detail_log_file, blood_log_file);

  // Log the aggregate headers
  ss << "DaysElapsed,Population,PfPR2to10,TreatmentCoverage,InfectedIndividuals,"
     << "ClinicalIndividuals,ClinicalU5,ClinicalO5,NewInfections,Treatment,"
     << "NonTreatment,TreatmentFailure,ParasiteClones,Theta,580yWeighted,"
     << "508yUnweighted,Plasmepsin2xCopyWeighted,Plasmepsin2xCopyUnweighted\n";
  aggregate_logger->info(ss.str());
  ss.str("");

#ifdef ENABLE_GENOTYPE_REPORTER
  // Log the detailed report headers
  ss << "DaysElapsed,Individual,";
  for (std::size_t id = 0; id < Model::CONFIG->genotype_db()->size(); id++) {
    auto genotype = (*Model::CONFIG->genotype_db())[id];
    ss << genotype->to_string() << ",";
  }
  ss << "\n";
  detail_logger->info(ss.str());
  ss.str("");
#endif

#ifdef ENABLE_BLOOD_REPORTER
  // Log the blood density report headers
  ss << "DaysElapsed,Individual,ParasitePopulation,C580Y,Density,Theta\n";
  blood_logger->info(ss.str());
  ss.str("");
#endif
}

void CellularReporter::monthly_report() {
  int _580yCount = 0;
  int infectedIndividuals = 0;
  int plasmepsinDoubleCopy = 0;
  int parasiteClones = 0;
  int population = 0;
  float _580yWeighted = 0.0;
  float plasmepsinDoubleCopyWeighted = 0.0;

  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    for (std::size_t ac = 0; ac < index->vPerson()[0][0].size(); ac++) {
      auto age_class = index->vPerson()[0][hs][ac];
      for (auto& person : age_class) {
        auto& parasites = *person->get_all_clonal_parasite_populations();
        population++;

        if (parasites.empty()) continue;

        infectedIndividuals++;

        for (auto& parasite_population : parasites) {
          parasiteClones++;

          if (parasite_population->genotype()->get_aa_sequence()[2] == 1) {
            _580yCount++;
            _580yWeighted += (1.0 / static_cast<double>(parasites.size()));
          }

          if (parasite_population->genotype()->get_aa_sequence()[3] == 1) {
            plasmepsinDoubleCopy++;
            plasmepsinDoubleCopyWeighted += (1.0 / static_cast<double>(parasites.size()));
          }
        }
      }
    }
  }

  auto clinical_all = Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[0];
  auto clinical_u5 = 0;
  for (auto ndx = 0; ndx < Model::get_config()->number_of_age_classes(); ndx++) {
    if (Model::get_config()->get_population_demographic().get_age_structure()[ndx] < 5) break;
    clinical_u5 += Model::get_mdc()->monthly_number_of_clinical_episode_by_location_age_class()[0][ndx];
  }

  ss << fmt::format(
      "{},{},{:.2f},{:.2f},{},{},{},{},{},{},{},{},{},{:.2f},{:.2f},{},{}\n",
      Model::get_scheduler()->current_time(), population,
      Model::get_mdc()->get_blood_slide_prevalence(0, ModelDataCollector::PFPR_2TO10_AGE_FROM, ModelDataCollector::PFPR_2TO10_AGE_TO) * 100.0,
      (Model::get_treatment_coverage()->p_treatment_under_5[0] +
       Model::get_treatment_coverage()->p_treatment_over_5[0]) /
          2.0,
      infectedIndividuals, clinical_all, clinical_u5, clinical_all - clinical_u5,
      Model::get_mdc()->monthly_number_of_new_infections_by_location()[0],
      Model::get_mdc()->monthly_number_of_treatment_by_location()[0],
      Model::get_mdc()->monthly_nontreatment_by_location()[0],
      Model::get_mdc()->monthly_treatment_failure_by_location()[0],
      parasiteClones, population_mean_theta(), _580yWeighted, _580yCount,
      plasmepsinDoubleCopyWeighted, plasmepsinDoubleCopy);

  spdlog::get("aggregate_reporter")->info(ss.str());
  ss.str("");

#ifdef ENABLE_GENOTYPE_REPORTER
  if (dayselapsed > (22 * 365 + 4) && dayselapsed < (23 * 365 + 5)) {
    detailed_report();
  }
#endif

#ifdef ENABLE_BLOOD_REPORTER
  if (dayselapsed > (11 * 365 + 3)) {
    blood_density_report();
  }
#endif
}

double CellularReporter::population_mean_theta() {
  auto theta = 0.0;
  auto population = 0;

  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    for (std::size_t ac = 0; ac < index->vPerson()[0][0].size(); ac++) {
      auto age_class = index->vPerson()[0][hs][ac];
      for (auto& person : age_class) {
        theta += person->get_immune_system()->get_current_value();
        population++;
      }
    }
  }

  return population > 0 ? (theta / population) : 0.0;
}
