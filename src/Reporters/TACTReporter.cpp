#include "TACTReporter.h"

#include <Configuration/Config.h>
#include <MDC/ModelDataCollector.h>
#include <Population/Population.h>
#include <Population/SingleHostClonalParasitePopulations.h>
#include <Simulation/Model.h>
#include <Treatment/Strategies/IStrategy.h>
#include <Treatment/Strategies/NestedMFTStrategy.h>
#include <Utils/Index/PersonIndexByLocationStateAgeClass.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Core/Scheduler/Scheduler.h"
#include "Parasites/Genotype.h"
#include "Reporters/Reporter.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Utility/ReporterUtils.h"

namespace {
// Emitted in place of the nested-MFT distribution columns when the active
// treatment strategy does not carry one.
constexpr int K_NOT_APPLICABLE = -1;
}  // namespace

void TACTReporter::initialize(int job_number, const std::string &path) {
  spdlog::info("TACTReporter initialized with job number {}", job_number);

  auto monthly_filename =
      fmt::format("{}tact_monthly_data_{}.{}", path, job_number, csv::EXTENSION);
  auto summary_filename =
      fmt::format("{}tact_summary_data_{}.{}", path, job_number, csv::EXTENSION);

  // Create console logger
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::get_level());
  auto console_logger = std::make_shared<spdlog::logger>("console_logger", console_sink);
  console_sink->set_pattern("[%^%l%$] %v");  // Highlight log level in console
  console_logger->set_level(spdlog::level::info);
  spdlog::register_logger(console_logger);

  // Create monthly report logger
  auto monthly_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(monthly_filename, true);
  auto monthly_logger = std::make_shared<spdlog::logger>("monthly_reporter", monthly_sink);
  monthly_sink->set_pattern(" %v");  // Timestamp for file logs
  monthly_logger->set_level(spdlog::level::info);
  spdlog::register_logger(monthly_logger);

  // Create summary report logger
  auto summary_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(summary_filename, true);
  auto summary_logger = std::make_shared<spdlog::logger>("summary_reporter", summary_sink);
  summary_sink->set_pattern("%v");  // Timestamp for file logs
  summary_logger->set_level(spdlog::level::info);
  spdlog::register_logger(summary_logger);

  // Set console logger as default
  spdlog::set_default_logger(console_logger);
}

void TACTReporter::before_run() {
  // output header for csv file
  ss << "TIME" << tsv::SEP << "PFPR" << tsv::SEP << "MUTATIONS" << tsv::SEP
     << "NUMBER_OF_TREATMENTS" << tsv::SEP << "NUMBER_OF_TREATMENT_FAILURES" << tsv::SEP
     << "NUMBER_OF_SYMPTOMATIC_CASES" << tsv::SEP;
  for (auto i = 0; i < Model::get_genotype_db()->size(); i++) {
    ss << "GENOTYPE_ID_" << i << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;
  for (auto i = 0; i < Model::get_mdc()->current_tf_by_therapy().size(); i++) {
    ss << "TF_THERAPY_" << i << tsv::SEP;
  }
  ss << "AVERAGE_TF_60" << tsv::SEP;
  ss << "PUBLIC_FRACTION" << tsv::SEP;
  ss << "PRIVATE_FRACTION";
  spdlog::get("monthly_reporter")->info("{}", ss.str());
  ss.str("");
}

void TACTReporter::monthly_report() {
  ss << Model::get_scheduler()->current_time() << tsv::SEP;

  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    ss << Model::get_mdc()->blood_slide_prevalence_by_location()[loc] * 100 << tsv::SEP;
  }

  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    ss << Model::get_mdc()->monthly_number_of_mutation_events_by_location()[loc] << tsv::SEP;
    ss << Model::get_mdc()->monthly_number_of_treatment_by_location()[loc] << tsv::SEP;
    ss << Model::get_mdc()->monthly_number_of_tf_by_location()[loc] << tsv::SEP;
    ss << Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[loc] << tsv::SEP;
  }

  output_genotype_frequency_3(
      static_cast<int>(Model::get_genotype_db()->size()),
      Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>());

  ss << tsv::GROUP_SEP;

  for (auto tf_by_therapy : Model::get_mdc()->current_tf_by_therapy()) {
    ss << tf_by_therapy << tsv::SEP;
  }

  ss << Model::get_mdc()->current_tf_by_location()[0] << tsv::SEP;

  // The type guard here already prevented a null dereference, but skipping the
  // columns entirely made the row width depend on the active strategy, which
  // breaks fixed-column parsing of the TSV. Emit placeholders instead.
  //
  // NovelDrugIntroductionStrategy derives from NestedMFTStrategy but reports a
  // different type, so the dynamic_cast rather than the type tag decides here.
  const auto* nested_strategy =
      dynamic_cast<NestedMFTStrategy*>(Model::get_treatment_strategy());
  if (nested_strategy != nullptr && nested_strategy->distribution.size() >= 2) {
    ss << nested_strategy->distribution[0] << tsv::SEP;
    ss << nested_strategy->distribution[1];
  } else {
    ss << K_NOT_APPLICABLE << tsv::SEP << K_NOT_APPLICABLE;
  }

  spdlog::get("monthly_reporter")->info("{}", ss.str());
  ss.str("");
}

void TACTReporter::after_run() {
  ss.str("");
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    ss << Model::get_config()->location_db()[loc].beta << tsv::SEP;
    if (Model::get_mdc()->eir_by_location_year()[loc].empty()) {
      ss << 0 << tsv::SEP;
    } else {
      ss << Model::get_mdc()->eir_by_location_year()[loc].back() << tsv::SEP;
    }
    ss << Model::get_treatment_coverage()->p_treatment_under_5[0] << tsv::SEP;
    ss << Model::get_mdc()->cumulative_number_treatments_by_location()[loc] << tsv::SEP;
    ss << Model::get_mdc()->cumulative_tf_by_location()[loc] << tsv::SEP;
    ss << Model::get_mdc()->cumulative_clinical_episodes_by_location()[loc] << tsv::SEP;
    ss << "FLT" << tsv::SEP;
    ss << "TACT" << tsv::SEP;
    ss << "importation" << tsv::SEP;
  }
  spdlog::get("summary_reporter")->info("{}", ss.str());
  ss.str("");
}

void TACTReporter::begin_time_step() {}

void TACTReporter::output_genotype_frequency_3(const int &number_of_genotypes,
                                               PersonIndexByLocationStateAgeClass* pi) {
  auto sum1_all = 0.0;
  std::vector<double> result3_all(number_of_genotypes, 0.0);
  const auto number_of_locations = pi->vPerson().size();
  const auto number_of_age_classes = pi->vPerson()[0][0].size();

  for (auto loc = 0; loc < number_of_locations; loc++) {
    std::vector<double> result3(number_of_genotypes, 0.0);
    auto sum1 = 0.0;

    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < number_of_age_classes; ac++) {
        const auto size = pi->vPerson()[loc][hs][ac].size();
        for (auto i = 0ULL; i < size; i++) {
          auto* person = pi->vPerson()[loc][hs][ac][i];

          if (!person->get_all_clonal_parasite_populations()->empty()) {
            sum1 += 1;
            sum1_all += 1;
          }

          std::map<int, int> individual_genotype_map;

          for (auto &parasite_population : *person->get_all_clonal_parasite_populations()) {
            const auto g_id = parasite_population->genotype()->genotype_id();
            if (!individual_genotype_map.contains(g_id)) {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] = 1;
            } else {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] += 1;
            }
          }

          for (const auto genotype : individual_genotype_map) {
            result3[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
            result3_all[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
          }
        }
      }
    }
    // output per location
    for (auto &occurence_by_loc : result3) {
      occurence_by_loc /= sum1;
      ss << (sum1 == 0 ? 0 : occurence_by_loc) << tsv::SEP;
    }
  }
}
