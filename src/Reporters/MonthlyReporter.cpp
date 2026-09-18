#include "MonthlyReporter.h"

#include <date/date.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Population/Population.h"
#include "Reporters/Reporter.h"
#include "Simulation/Model.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Utility/ReporterUtils.h"
#include "Utils/Constants.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "Utils/Random.h"

namespace fs = std::filesystem;

MonthlyReporter::MonthlyReporter() = default;

void MonthlyReporter::initialize(int job_number, const std::string &path) {
  // Define file paths
  std::string gene_freq_path = fmt::format("{}/gene_freq_{}.txt", path, job_number);
  std::string monthly_data_path = fmt::format("{}/monthly_data_{}.txt", path, job_number);
  std::string summary_data_path = fmt::format("{}/summary_{}.txt", path, job_number);
  std::string gene_db_path = fmt::format("{}/gene_db_{}.txt", path, job_number);

  // Remove old files if they exist
  fs::remove(gene_freq_path);
  fs::remove(monthly_data_path);
  fs::remove(summary_data_path);
  fs::remove(gene_db_path);

  // Create separate loggers for each report type
  gene_freq_logger = spdlog::basic_logger_mt("gene_freq", gene_freq_path);
  monthly_data_logger = spdlog::basic_logger_mt("monthly_data", monthly_data_path);
  summary_data_logger = spdlog::basic_logger_mt("summary", summary_data_path);
  gene_db_logger = spdlog::basic_logger_mt("gene_db", gene_db_path);

  // Set log pattern to only include the raw message (removes timestamps and log levels)
  gene_freq_logger->set_pattern("%v");
  monthly_data_logger->set_pattern("%v");
  summary_data_logger->set_pattern("%v");
  gene_db_logger->set_pattern("%v");

  // Set up a default console logger
  auto console_logger = spdlog::get("console");
  if (!console_logger) { console_logger = spdlog::stdout_color_mt("console"); }
  spdlog::set_default_logger(console_logger);  // Make console logger the default
  // spdlog::set_pattern("[%H:%M:%S] %v");  // Format console log with time

  // Optional: Set logger flush levels
  gene_freq_logger->flush_on(spdlog::level::info);
  monthly_data_logger->flush_on(spdlog::level::info);
  summary_data_logger->flush_on(spdlog::level::info);
  gene_db_logger->flush_on(spdlog::level::info);
}

void MonthlyReporter::before_run() {}

void MonthlyReporter::begin_time_step() {}

void MonthlyReporter::monthly_report() {
  std::stringstream ss;

  ss << Model::get_scheduler()->current_time() << tsv::SEP;
  ss << Model::get_scheduler()->get_unix_time() << tsv::SEP;
  ss << Model::get_scheduler()->get_current_date_string() << tsv::SEP;
  ss << Model::get_config()->get_seasonality_settings().get_seasonal_factor(
      Model::get_scheduler()->get_calendar_date(), 0)
     << tsv::SEP;
  ss << Model::get_treatment_coverage()->get_probability_to_be_treated(0, 1) << tsv::SEP;
  ss << Model::get_treatment_coverage()->get_probability_to_be_treated(0, 10) << tsv::SEP;
  ss << Model::get_population()->size() << tsv::SEP;
  ss << tsv::GROUP_SEP;

  print_eir_pfpr_by_location(ss);
  ss << tsv::GROUP_SEP;
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_new_infections_by_location()[loc] << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_treatment_by_location()[loc] << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[loc] << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;

  // Log monthly data
  monthly_data_logger->info(ss.str());

  // Log gene frequency
  std::stringstream gene_freq_ss;
  ReporterUtils::output_genotype_frequency3(
      gene_freq_ss, static_cast<int>(Model::get_genotype_db()->size()),
      Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>());
  gene_freq_logger->info(gene_freq_ss.str());
}

void MonthlyReporter::after_run() {
  std::stringstream ss;
  ss << Model::get_random()->get_seed() << tsv::SEP << Model::get_config()->number_of_locations()
     << tsv::SEP;
  ss << Model::get_config()->location_db()[0].beta << tsv::SEP;
  ss << Model::get_config()->location_db()[0].population_size << tsv::SEP;
  print_eir_pfpr_by_location(ss);

  ss << tsv::GROUP_SEP;
  ss << Model::get_treatment_strategy()->id << tsv::SEP;

  // Calculate and log NTF
  const auto total_time_in_years =
      (Model::get_scheduler()->current_time()
       - Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period())
      / static_cast<double>(Constants::DAYS_IN_YEAR);

  auto sum_ntf = 0.0;
  uint64_t pop_size = 0;
  for (auto location = 0; location < Model::get_config()->number_of_locations(); location++) {
    sum_ntf += Model::get_mdc()->cumulative_ntf_by_location()[location];
    pop_size += Model::get_mdc()->popsize_by_location()[location];
  }
  ss << (sum_ntf * 100 / static_cast<double>(pop_size)) / total_time_in_years << tsv::SEP;

  summary_data_logger->info(ss.str());

  // Log gene database
  for (const auto &genotype : *Model::get_genotype_db()) {
    gene_db_logger->info("{}{}{}", genotype->genotype_id(), tsv::SEP, genotype->get_aa_sequence());
  }
}

void MonthlyReporter::print_eir_pfpr_by_location(std::stringstream &ss) {
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    if (Model::get_mdc()->eir_by_location_year()[loc].empty()) {
      ss << 0 << tsv::SEP;
      // spdlog::info("print_EIR_PfPR_by_location {}: EIR_by_location_year is empty", loc);
    } else {
      ss << Model::get_mdc()->eir_by_location_year()[loc].back() << tsv::SEP;
      // spdlog::info("print_EIR_PfPR_by_location {}: EIR_by_location_year {:.8f}", loc,
      // Model::get_mdc()->EIR_by_location_year()[loc].back());
    }
    ss << tsv::GROUP_SEP;
    ss << Model::get_mdc()->get_blood_slide_prevalence(loc, 2, 10) * 100 << tsv::SEP;
    ss << Model::get_mdc()->get_blood_slide_prevalence(loc, 0, 5) * 100 << tsv::SEP;
    ss << Model::get_mdc()->blood_slide_prevalence_by_location()[loc] * 100 << tsv::SEP;
  }
}
