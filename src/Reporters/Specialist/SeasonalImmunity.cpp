/*
 * SeasonalImmunity.cpp
 *
 * Implements the SeasonalImmunity class.
 */
#include "SeasonalImmunity.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Spatial/GIS/SpatialData.h"
#include "Utils/Helpers/RunningMedian.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "spdlog/sinks/basic_file_sink.h"

void SeasonalImmunity::initialize(int job_number, const std::string &path) {
  // Start by building our look-up
  build_lookup();

  spdlog::info("SeasonalImmunityReporter initialized with job number {}", job_number);

  auto monthly_filename =
      fmt::format("{}seasonal_immunity_monthly_data_{}.{}", path, job_number, csv::EXTENSION);

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

  // Set console logger as default
  spdlog::set_default_logger(console_logger);

  // Log the seasonal headers
  ss << "DaysElapsed" << csv::SEP << "ClimaticZone" << csv::SEP << "Population" << csv::SEP
     << "MedianTheta" << csv::SEP << "MeanTheta" << csv::SEP << "InfectedIndividuals" << csv::SEP
     << "ClinicalIndividuals" << csv::SEP << "NewInfections" << csv::SEP << "Treatments" << csv::SEP
     << "NonTreatment" << csv::SEP << "TreatmentFailure" << csv::SEP << "ParasiteClones" << csv::SEP
     << "Multiclonal" << csv::SEP << "580yWeighted" << csv::SEP << "580yUnweighted" << csv::SEP
     << "580yMulticlonal" << csv::SEP << csv::END_LINE;
  spdlog::get("monthly_reporter")->info(ss.str());
  ss.str("");

  // Note that we are running
  spdlog::info("SeasonalImmunity reporter initialized, writing to: {}", monthly_filename);
}

void SeasonalImmunity::monthly_report() {
  if (!Model::get_mdc()->recording_data()) { return; }

  std::vector<int> population(lookup_allocation, 0);
  std::vector<RunningMedian<double>> median_theta(lookup_allocation, RunningMedian<double>());
  std::vector<double> mean_theta(lookup_allocation, 0);
  std::vector<int> new_infections(lookup_allocation, 0);
  std::vector<int> treatments(lookup_allocation, 0);
  std::vector<int> nontreatment(lookup_allocation, 0);
  std::vector<int> treatment_failure(lookup_allocation, 0);
  std::vector<int> clinical_individuals(lookup_allocation, 0);
  std::vector<int> infected_individuals(lookup_allocation, 0);
  std::vector<int> parasite_clones(lookup_allocation, 0);
  std::vector<int> multiclonal(lookup_allocation, 0);
  std::vector<int> unweighted_580y(lookup_allocation, 0);
  std::vector<int> multiclonal_580y(lookup_allocation, 0);
  std::vector<double> weighted_580y(lookup_allocation, 0);

  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto age_classes = index->vPerson()[0][0].size();

  for (auto location = 0; location < index->vPerson().size(); location++) {
    auto zone = lookup[location];
    new_infections[zone] +=
        Model::get_mdc()->monthly_number_of_new_infections_by_location()[location];
    treatments[zone] += Model::get_mdc()->monthly_number_of_treatment_by_location()[location];
    nontreatment[zone] += Model::get_mdc()->monthly_nontreatment_by_location()[location];
    treatment_failure[zone] += Model::get_mdc()->monthly_treatment_failure_by_location()[location];
    clinical_individuals[zone] +=
        Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[location];

    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (unsigned int ac = 0; ac < age_classes; ac++) {
        for (auto &person : index->vPerson()[location][hs][ac]) {
          population[zone]++;
          auto theta = person->get_immune_system()->get_current_value();
          mean_theta[zone] += theta;
          median_theta[zone].push(theta);
          auto &parasites = *person->get_all_clonal_parasite_populations();
          if (parasites.empty()) { continue; }
          auto size = static_cast<int>(parasites.size());
          infected_individuals[zone]++;
          if (size > 1) { multiclonal[zone]++; }
          parasite_clones[zone] += size;
          for (auto ndx = 0; ndx < size; ndx++) {
            auto* parasite = parasites[ndx];
            if (parasite->genotype()->get_aa_sequence()[2] == 1) {
              unweighted_580y[zone]++;
              weighted_580y[zone] += (1 / static_cast<double>(size));
              if (size > 1) { multiclonal_580y[zone]++; }
            }
          }
        }
      }
    }
  }

  for (int zone = 0; zone < lookup_allocation; zone++) {
    ss << Model::get_scheduler()->current_time() << csv::SEP << zone << csv::SEP << population[zone]
       << csv::SEP << median_theta[zone].getMedian() << csv::SEP
       << mean_theta[zone] / population[zone] << csv::SEP << infected_individuals[zone] << csv::SEP
       << clinical_individuals[zone] << csv::SEP << new_infections[zone] << csv::SEP
       << treatments[zone] << csv::SEP << nontreatment[zone] << csv::SEP << treatment_failure[zone]
       << csv::SEP << parasite_clones[zone] << csv::SEP << multiclonal[zone] << csv::SEP
       << weighted_580y[zone] << csv::SEP << unweighted_580y[zone] << csv::SEP
       << multiclonal_580y[zone] << csv::SEP << csv::END_LINE;
  }
  spdlog::get("monthly_reporter")->info(ss.str());
  ss.str("");
}

void SeasonalImmunity::build_lookup() {
  // Get the raster data and make sure it is valid
  AscFile* raster =
      Model::get_spatial_data()->get_raster(SpatialData::SpatialFileType::ECOCLIMATIC);
  if (raster == nullptr) {
    spdlog::error(
        " The Seasonal Immunity reporter cannot be run without an "
        "associated climate raster.");
    throw std::runtime_error(
        "SeasonalImmunity reporter without underlying raster is not "
        "supported.");
  }

  // Load the value into the lookup based on the raster
  for (int row = 0; row < raster->nrows; row++) {
    for (int col = 0; col < raster->ncols; col++) {
      // Pass if we have no data
      if (raster->data[row][col] == raster->nodata_value) { continue; }

      // Verify the zone
      auto zone = static_cast<int>(raster->data[row][col]);
      if (zone < 0) {
        throw std::out_of_range(
            fmt::format("Raster value at row: {}, col: {} is less than zero.", row, col));
      }

      // Set the value
      lookup.emplace_back(zone);
      lookup_allocation = (lookup_allocation < zone) ? zone : lookup_allocation;
    }
  }

  // Update the lookup allocation by one to account for indexing
  lookup_allocation++;
}
