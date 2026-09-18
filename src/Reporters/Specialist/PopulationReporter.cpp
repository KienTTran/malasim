/*
 * PopulationReporter.cpp
 *
 * Implement the PopulationReporter class.
 */
#include "PopulationReporter.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "spdlog/sinks/basic_file_sink.h"

void PopulationReporter::initialize(int job_number, const std::string &path) {
  spdlog::info("PopulationReporter initialized with job number {}", job_number);

  auto monthly_filename =
      fmt::format("{}population_monthly_data_{}.{}", path, job_number, csv::EXTENSION);

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

  // Log the headers
  ss << "dayselapsed" << csv::SEP << "population" << csv::SEP << "births" << csv::SEP << "deaths"
     << csv::SEP << "malariadeaths" << csv::SEP << "cases" << csv::SEP << "treatments" << csv::SEP
     << csv::END_LINE;
  spdlog::get("monthly_reporter")->info(ss.str());
  ss.str("");
}

void PopulationReporter::monthly_report() {
  uint64_t population = 0;
  uint64_t births = 0;
  uint64_t deaths = 0;
  uint64_t malaria_deaths = 0;
  uint64_t cases = 0;
  uint64_t treatments = 0;

  // Calculate the summary data
  for (std::size_t location = 0; location < Model::get_config()->number_of_locations();
       location++) {
    population += Model::get_population()->size_at(static_cast<int>(location));
    births += Model::get_mdc()->births_by_location()[location];
    deaths += Model::get_mdc()->deaths_by_location()[location];
    malaria_deaths += Model::get_mdc()->malaria_deaths_by_location()[location];
    cases += Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[location];
    treatments += Model::get_mdc()->monthly_number_of_treatment_by_location()[location];
  }

  // Log the entry
  ss << Model::get_scheduler()->current_time() << csv::SEP << population << csv::SEP << births
     << csv::SEP << deaths << csv::SEP << malaria_deaths << csv::SEP << cases << csv::SEP
     << treatments << csv::SEP << csv::END_LINE;
  spdlog::get("monthly_reporter")->info(ss.str());
  ss.str("");
}
