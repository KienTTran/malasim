
#include "MMCReporter.h"

#include <spdlog/sinks/basic_file_sink.h>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Utility/ReporterUtils.h"
#include "Utils/Constants.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "Utils/Random.h"

MMCReporter::MMCReporter() = default;
#include <spdlog/sinks/stdout_color_sinks.h>

void MMCReporter::initialize(int job_number, const std::string &path) {
  auto* mdc = Model::get_mdc();
  const auto console_log_level = spdlog::get_level();
  ReporterUtils::initialize_moi_file_logger();

  spdlog::info("MMCReporter initialized with job number {}", job_number);

  auto monthly_filename = fmt::format("{}mmc_monthly_data_{}.{}", path, job_number, csv::EXTENSION);
  auto summary_filename = fmt::format("{}mmc_summary_data_{}.{}", path, job_number, csv::EXTENSION);

  // Create console logger
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(console_log_level);
  auto console_logger = std::make_shared<spdlog::logger>("console_logger", console_sink);
  console_sink->set_pattern("[%^%l%$] %v");  // Highlight log level in console
  console_logger->set_level(spdlog::level::info);
  spdlog::register_logger(console_logger);

  // Create monthly report logger
  auto monthly_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(monthly_filename, true);
  auto monthly_logger = std::make_shared<spdlog::logger>("monthly_reporter", monthly_sink);
  monthly_sink->set_pattern("%v");  // Timestamp for file logs
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

void MMCReporter::before_run() {
  // // std::cout << "MMC Reporter" << std::endl;
  // for (auto genotype : (*Model::get_genotype_db())){
  //   std::cout << *genotype.second << std::endl;
  // }
}

void MMCReporter::begin_time_step() {}

void MMCReporter::print_treatment_failure_rate_by_therapy() {
  auto* mdc = Model::get_mdc();
  for (auto tf_by_therapy : mdc->current_tf_by_therapy()) { ss << tf_by_therapy << tsv::SEP; }
}

void MMCReporter::print_ntf_by_location() {
  auto* mdc = Model::get_mdc();
  auto* config = Model::get_config();
  double sum_ntf = 0.0;
  uint64_t pop_size = 0;
  for (auto location = 0; location < config->number_of_locations(); location++) {
    sum_ntf += mdc->cumulative_ntf_by_location()[location];
    pop_size += mdc->popsize_by_location()[location];
  }

  ss << (sum_ntf * 100.0 / static_cast<double>(pop_size)) << tsv::SEP;
}

void MMCReporter::monthly_report() {
  auto* scheduler = Model::get_scheduler();
  auto* mdc = Model::get_mdc();
  auto* config = Model::get_config();
  auto* treatment_coverage = Model::get_treatment_coverage();
  auto* population = Model::get_population();

  ss << scheduler->current_time() << tsv::SEP;
  ss << scheduler->get_unix_time() << tsv::SEP;
  ss << scheduler->get_calendar_date() << tsv::SEP;
  ss << config->get_seasonality_settings().get_seasonal_factor(scheduler->get_calendar_date(), 0)
     << tsv::SEP;
  ss << treatment_coverage->get_probability_to_be_treated(0, 1) << tsv::SEP;
  ss << treatment_coverage->get_probability_to_be_treated(0, 10) << tsv::SEP;
  ss << population->size() << tsv::SEP;
  ss << tsv::GROUP_SEP;

  print_eir_pfpr_by_location();
  ss << tsv::GROUP_SEP;
  for (auto loc = 0; loc < config->number_of_locations(); loc++) {
    ss << mdc->monthly_number_of_treatment_by_location()[loc] << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;
  for (auto loc = 0; loc < config->number_of_locations(); loc++) {
    ss << mdc->monthly_number_of_clinical_episode_by_location()[loc] << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;

  ReporterUtils::output_genotype_frequency3(
      ss, static_cast<int>(Model::get_genotype_db()->size()),
      population->get_person_index<PersonIndexByLocationStateAgeClass>());

  ss << tsv::GROUP_SEP;
  print_ntf_by_location();
  ss << tsv::GROUP_SEP;
  print_treatment_failure_rate_by_therapy();
  ss << mdc->current_tf_by_location()[0];
  // CLOG(INFO, "monthly_reporter") << ss.str();
  spdlog::get("monthly_reporter")->info("{}", ss.str());
  ss.str("");
}

void MMCReporter::after_run() {
  auto* mdc = Model::get_mdc();
  auto* scheduler = Model::get_scheduler();
  auto* config = Model::get_config();
  auto* population = Model::get_population();

  ss.str("");
  ss << Model::get_random()->get_seed() << tsv::SEP << config->number_of_locations() << tsv::SEP;
  ss << config->location_db()[0].beta << tsv::SEP;
  ss << config->location_db()[0].population_size << tsv::SEP;
  print_eir_pfpr_by_location();

  ss << tsv::GROUP_SEP;
  // output last strategy information
  ss << Model::get_treatment_strategy()->id << tsv::SEP;

  // output NTF
  const auto total_time_in_years =
      (scheduler->current_time()
       - config->get_simulation_timeframe().get_start_of_comparison_period())
      / static_cast<double>(Constants::DAYS_IN_YEAR);

  auto sum_ntf = 0.0;
  uint64_t pop_size = 0;
  for (auto location = 0; location < config->number_of_locations(); location++) {
    sum_ntf += mdc->cumulative_ntf_by_location()[location];
    pop_size += mdc->popsize_by_location()[location];
  }

  ss << (sum_ntf * 100 / static_cast<double>(pop_size)) / total_time_in_years << tsv::SEP;
  ss << tsv::GROUP_SEP;

  ss << mdc->cumulative_number_treatments_by_location()[0] << tsv::SEP;
  ss << mdc->cumulative_tf_by_location()[0] << tsv::SEP;
  ss << mdc->cumulative_clinical_episodes_by_location()[0] << tsv::SEP;

  ss << tsv::GROUP_SEP;
  // print # mutation events of first 10 years
  int number_of_years = mdc->number_of_mutation_events_by_year().size() >= 11
                            ? 11
                            : static_cast<int>(mdc->number_of_mutation_events_by_year().size());
  for (int i = 0; i < number_of_years; ++i) {
    ss << mdc->number_of_mutation_events_by_year()[i] << tsv::SEP;
  }

  // CLOG(INFO, "summary_reporter") << ss.str();
  spdlog::get("summary_reporter")->info("{}", ss.str());
  ss.str("");

  // Report MOI
  ReporterUtils::output_moi(ss, population->get_person_index<PersonIndexByLocationStateAgeClass>());
}

void MMCReporter::print_eir_pfpr_by_location() {
  auto* mdc = Model::get_mdc();
  auto* config = Model::get_config();
  for (auto loc = 0; loc < config->number_of_locations(); ++loc) {
    //
    // EIR
    if (mdc->eir_by_location_year()[loc].empty()) {
      ss << 0 << tsv::SEP;
    } else {
      ss << mdc->eir_by_location_year()[loc].back() << tsv::SEP;
    }

    // pfpr <5 and all
    ss << mdc->get_blood_slide_prevalence(loc, 0, 5) * 100 << tsv::SEP;
    ss << mdc->get_blood_slide_prevalence(loc, 2, 10) * 100 << tsv::SEP;
    ss << mdc->blood_slide_prevalence_by_location()[loc] * 100 << tsv::SEP;
    //    std::cout << population->size() << "\t"
    //              << mdc->blood_slide_prevalence_by_location()[loc] * 100 <<
    //              std::endl;
  }
}
