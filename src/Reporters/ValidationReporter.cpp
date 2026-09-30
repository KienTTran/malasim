#include "ValidationReporter.h"

#include <date/date.h>
#include <spdlog/sinks/stdout_color_sinks.h>  // Console logger

#include <filesystem>  // For file operations

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Mosquito/Mosquito.h"
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

ValidationReporter::ValidationReporter() = default;

void ValidationReporter::initialize(int job_number, const std::string &path) {
  // Define file paths
  std::string monthly_data_path =
      fmt::format("{}/validation_monthly_data_{}.txt", path, job_number);
  std::string summary_data_path = fmt::format("{}/validation_summary_{}.txt", path, job_number);
  std::string gene_freq_path = fmt::format("{}/validation_gene_freq_{}.txt", path, job_number);
  std::string gene_db_path = fmt::format("{}/validation_gene_db_{}.txt", path, job_number);

  // Remove old files if they exist
  fs::remove(monthly_data_path);
  fs::remove(summary_data_path);
  fs::remove(gene_freq_path);
  fs::remove(gene_db_path);

  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()) {
    monthly_mutation_path = fmt::format("{}/validation_monthly_mutation_{}.txt", path, job_number);
    mosquito_res_count_path =
        fmt::format("{}/validation_mosquito_res_count_{}.txt", path, job_number);
    fs::remove(monthly_mutation_path);
    fs::remove(mosquito_res_count_path);
  }

  // Create separate loggers for each report type
  monthly_data_logger = spdlog::basic_logger_mt("validation_monthly_data", monthly_data_path);
  summary_data_logger = spdlog::basic_logger_mt("validation_summary", summary_data_path);
  gene_freq_logger = spdlog::basic_logger_mt("validation_gene_freq", gene_freq_path);
  gene_db_logger = spdlog::basic_logger_mt("validation_gene_db", gene_db_path);

  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()) {
    monthly_mutation_logger =
        spdlog::basic_logger_mt("validation_monthly_mutation", monthly_mutation_path);
    mosquito_res_count_logger =
        spdlog::basic_logger_mt("validation_mosquito_res_count", mosquito_res_count_path);
  }

  // Set log pattern to only include the raw message (removes timestamps and log levels)
  monthly_data_logger->set_pattern("%v");
  summary_data_logger->set_pattern("%v");
  gene_freq_logger->set_pattern("%v");
  gene_db_logger->set_pattern("%v");
  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()) {
    monthly_mutation_logger->set_pattern("%v");
    mosquito_res_count_logger->set_pattern("%v");
  }

  // Set up a default console logger
  auto console_logger = spdlog::get("console");
  if (!console_logger) { console_logger = spdlog::stdout_color_mt("console"); }
  // console_logger->set_pattern("[%H:%M:%S] %v");  // Format console log with time
  spdlog::set_default_logger(console_logger);  // Make console logger the default

  // Optional: Set logger flush levels
  monthly_data_logger->flush_on(spdlog::level::info);
  summary_data_logger->flush_on(spdlog::level::info);
  gene_freq_logger->flush_on(spdlog::level::info);
  gene_db_logger->flush_on(spdlog::level::info);
  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()) {
    monthly_mutation_logger->flush_on(spdlog::level::info);
    mosquito_res_count_logger->flush_on(spdlog::level::info);
  }
}

void ValidationReporter::before_run() {}

void ValidationReporter::begin_time_step() {}

void ValidationReporter::monthly_report() {
  // spdlog::info("monthly_report {} {} {} {} {}",Model::get_scheduler()->current_time(), 0,
  // Model::get_population()->size_at(0),
  //   Constants::DAYS_IN_YEAR,Model::get_mdc()->person_days_by_location_year()[0]);
  std::stringstream ss;

  ss << Model::get_scheduler()->current_time() << tsv::SEP;
  ss << Model::get_scheduler()->get_unix_time() << tsv::SEP;
  ss << Model::get_scheduler()->get_current_date_seperated_string() << tsv::SEP;
  ss << Model::get_config()->get_seasonality_settings().get_seasonal_factor(
      Model::get_scheduler()->get_calendar_date(), 0)
     << tsv::SEP;
  ss << Model::get_treatment_coverage()->get_probability_to_be_treated(0, 1) << tsv::SEP;
  ss << Model::get_treatment_coverage()->get_probability_to_be_treated(0, 10) << tsv::SEP;
  ss << Model::get_population()->size() << tsv::SEP;
  ss << tsv::GROUP_SEP;            // 9 - index 0
  print_eir_pfpr_by_location(ss);  // 11
  ss << tsv::GROUP_SEP;            // 15
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_new_infections_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 17
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_treatment_by_location()[loc]
       << tsv::SEP;        // Incidence
    ss << tsv::GROUP_SEP;  // 19
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  /// 21
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto moi : Model::get_mdc()->multiple_of_infection_by_location()[loc]) {
      ss << moi << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 32
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()->blood_slide_prevalence_by_location_age_group()[loc][ac] << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 48
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->blood_slide_prevalence_by_location_age()[loc][age] << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 129
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->current_tf_by_location()[loc] << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;  // 131
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_mutation_events_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 133
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()->number_of_treatments_by_location_age_year()[loc][ac] << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 149
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->total_number_of_bites_by_location()[loc] << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;  // 151
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->total_number_of_bites_by_location_year()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 153
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->today_number_of_treatments_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 155
  }
  ss << Model::get_mdc()->current_number_of_mutation_events_in_this_year() << tsv::SEP;
  ss << tsv::GROUP_SEP;  // 157
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    if ((Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
         + Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::CLINICAL])
        == 0) {
      ss << 0 << tsv::SEP;
    } else {
      ss << Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
                / (Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
                   + Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::CLINICAL])
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 159
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << Model::get_mdc()->popsize_by_location_age()[loc][age] << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 240
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << Model::get_mdc()->monthly_number_of_clinical_episode_by_location_age()[loc][age]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 321
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      int all_infected_pop =
          Model::get_mdc()->popsize_by_location_hoststate_age_class()[loc][Person::ASYMPTOMATIC][ac]
          + Model::get_mdc()->popsize_by_location_hoststate_age_class()[loc][Person::CLINICAL][ac];
      if (all_infected_pop == 0) {
        ss << 0 << tsv::SEP;
      } else {
        ss << std::setprecision(6)
           << Model::get_mdc()->number_of_clinical_by_location_age_group()[loc][ac]
                  / static_cast<double>(all_infected_pop)
           << tsv::SEP;
      }
    }
    ss << tsv::GROUP_SEP;  // 337
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->total_immune_by_location()[loc]
              / static_cast<double>(Model::get_population()->size_at(loc))
       << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 339
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    int all_infected_pop =
        Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
        + Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::CLINICAL];
    if (all_infected_pop == 0) {
      ss << 0 << tsv::SEP;
    } else {
      ss << std::setprecision(6)
         << Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[loc]
                / static_cast<double>(all_infected_pop)
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 341
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_ntf_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 343
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_tf_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 345
  }  // 417
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_number_treatments_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 347
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_tf_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 349
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_clinical_episodes_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 351
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_untreated_cases_by_location_age_year()[loc][age]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 432
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_deaths_by_location_age_year()[loc][age] << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 513
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_malaria_deaths_treated_by_location_age_year()[loc][age]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 594
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_malaria_deaths_non_treated_by_location_age_year()[loc][age]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 675
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->total_immune_by_location_age()[loc][age] << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 756
  }
  for (auto tf_by_therapy : Model::get_mdc()->current_tf_by_therapy()) {
    ss << tf_by_therapy << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;  // 772
  for (int t_id = 0; t_id < Model::get_therapy_db().size(); t_id++) {
    int n_treaments = Model::get_mdc()->number_of_treatments_with_therapy_id()[t_id];
    int n_success = Model::get_mdc()->number_of_treatments_success_with_therapy_id()[t_id];
    int n_fail = Model::get_mdc()->number_of_treatments_fail_with_therapy_id()[t_id];
    double p_success = (n_treaments == 0) ? 0 : n_success * 100.0 / n_treaments;
    ss << n_treaments << tsv::SEP << n_success << tsv::SEP << n_fail << tsv::SEP << p_success
       << tsv::SEP;
  }
  ss << tsv::GROUP_SEP;  // 833
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << Model::get_mdc()->cumulative_clinical_episodes_by_location_age()[loc][age] << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 914
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->progress_to_clinical_in_7d_counter[loc].total << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 916
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->progress_to_clinical_in_7d_counter[loc].recrudescence << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 918
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->progress_to_clinical_in_7d_counter[loc].new_infection << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 920
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_recrudescence_treatment_by_location()[loc]
       << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 922
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()
                ->monthly_number_of_recrudescence_treatment_by_location_age_class()[loc][ac]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 938
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->monthly_number_of_recrudescence_treatment_by_location_age()[loc][age]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  /// 1019
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_treatment_failure_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 1021
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()->monthly_treatment_failure_by_location_age_class()[loc][ac]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 1137
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->total_number_of_bites_by_location()[loc] << tsv::SEP;
    ss << Model::get_mdc()->total_number_of_bites_by_location_year()[loc] << tsv::SEP;
    ss << Model::get_mdc()->person_days_by_location_year()[loc] << tsv::SEP;
    ss << Model::get_population()->current_force_of_infection_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 1142
  }
  const auto age_index_count =
      static_cast<int>(Model::get_config()
                           ->get_epidemiological_parameters()
                           .get_age_based_probability_of_seeking_treatment()
                           .get_ages()
                           .size());
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto idx = 0; idx < (age_index_count > 0 ? age_index_count : 1); ++idx) {
      ss << Model::get_mdc()
                ->monthly_number_of_people_seeking_treatment_by_location_age_index()[loc][idx]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 1158
  }
  monthly_data_logger->info(ss.str());

  std::stringstream gene_freq_ss;
  //    ReporterUtils::output_genotype_frequency3(gene_freq_ss, Model::get_genotype_db()->size(),
  //                                              Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>());
  ReporterUtils::output_genotype_frequency3(
      gene_freq_ss, static_cast<int>(Model::get_genotype_db()->size()),
      Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>());

  gene_freq_logger->info(gene_freq_ss.str());
  // prmc_freq_logger->info(prmc_freq_ss.str());

  ss.str("");

  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()) {
    int sum = 0;
    for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
      sum += static_cast<int>(Model::get_mdc()->mutation_tracker[loc].size());
      for (auto &yearly_data : Model::get_mdc()->mutation_tracker[loc]) {
        ss << std::get<0>(yearly_data) << tsv::SEP;
        ss << std::get<1>(yearly_data) << tsv::SEP;
        ss << std::get<2>(yearly_data) << tsv::SEP;
        ss << std::get<3>(yearly_data) << tsv::SEP;
        ss << std::get<4>(yearly_data) << tsv::SEP;
        ss << std::get<5>(yearly_data) << '\n';
      }
    }
    if (sum > 0) {
      monthly_mutation_logger->info(ss.str());
      for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
        Model::get_mdc()->mutation_tracker[loc].clear();
      }
    }

    ss.str("");
    sum = 0;
    for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
      sum += static_cast<int>(
          Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc].size());
      for (auto &genotype_tracked :
           Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc]) {
        ss << std::get<0>(genotype_tracked) << tsv::SEP;
        ss << std::get<1>(genotype_tracked) << tsv::SEP;
        ss << std::get<2>(genotype_tracked) << tsv::SEP;
        ss << std::get<3>(genotype_tracked) << '\n';
      }
    }
    if (sum > 0) {
      mosquito_res_count_logger->info(ss.str());
      for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
        Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc].clear();
      }
    }
  }
}

void ValidationReporter::after_run() {
  std::stringstream ss;

  ss.str("");
  ss << Model::get_random()->get_seed() << tsv::SEP << Model::get_config()->number_of_locations()
     << tsv::SEP;
  ss << Model::get_config()->location_db()[0].beta << tsv::SEP;
  ss << Model::get_config()->location_db()[0].population_size << tsv::SEP;
  print_eir_pfpr_by_location(ss);
  ss << tsv::GROUP_SEP;  // 9
  // output last strategy information
  ss << Model::get_treatment_strategy()->id << tsv::SEP;  // 10
  // output NTF
  const auto total_time_in_years =
      (Model::get_scheduler()->current_time()
       - Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period())
      / static_cast<double>(Constants::DAYS_IN_YEAR);
  auto sum_ntf = 0.0;
  uint64_t pop_size = 0;
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    sum_ntf += Model::get_mdc()->cumulative_ntf_by_location()[loc];
    pop_size += Model::get_mdc()->popsize_by_location()[loc];
  }
  ss << (sum_ntf * 100 / static_cast<double>(pop_size)) / total_time_in_years << tsv::SEP;
  ss << tsv::GROUP_SEP;  // 12
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_clinical_episodes_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 14
  }
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->percentage_bites_on_top_20_by_location()[loc] * 100 << "%" << tsv::SEP;
    //        std::cout << Model::get_mdc()->percentage_bites_on_top_20_by_location()[loc] * 100 <<
    //        "%" << "\t";
    ss << tsv::GROUP_SEP;  // 16
  }
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    double location_ntf = Model::get_mdc()->cumulative_ntf_by_location()[loc] * 100
                          / static_cast<double>(Model::get_mdc()->popsize_by_location()[loc]);
    location_ntf /= total_time_in_years;
    ss << location_ntf << tsv::SEP;
    //        std::cout << location_NTF << "\t";
    ss << tsv::GROUP_SEP;  // 18
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << static_cast<double>(
                Model::get_mdc()->cumulative_clinical_episodes_by_location_age()[loc][age])
                / total_time_in_years / Model::get_mdc()->popsize_by_location_age()[loc][age]
         << tsv::SEP;
    }
    ss << tsv::GROUP_SEP;  // 99
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_number_treatments_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 101
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_tf_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 103
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_clinical_episodes_by_location()[loc] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 105
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->mosquito_recombination_events_count()[loc][0] << tsv::SEP;
    ss << Model::get_mdc()->mosquito_recombination_events_count()[loc][1] << tsv::SEP;
    ss << tsv::GROUP_SEP;  // 107
  }
  summary_data_logger->info(ss.str());

  for (const auto &genotype : *(Model::get_genotype_db())) {
    gene_db_logger->info("{}{}{}", genotype->genotype_id(), tsv::SEP, genotype->aa_sequence);
    // prmc_db_logger->info("{}{}{}",g_id,sep,genotype->aa_sequence);
  }

  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()) {
    for (const auto &genotype : *(Model::get_genotype_db())) {
      spdlog::debug("{}:{}", genotype->aa_sequence, genotype->daily_fitness_multiple_infection);
    }
    for (int resistant_drug_pair_id = 0;
         resistant_drug_pair_id < Model::get_mosquito()->resistant_drug_list.size();
         resistant_drug_pair_id++) {
      auto drugs = Model::get_mosquito()->resistant_drug_list[resistant_drug_pair_id].second;
      for (const auto &genotype : *(Model::get_genotype_db())) {
        if (resistant_drug_pair_id < 3) {
          spdlog::debug(fmt::format(
              "resistant_drug_pair_id: {} {}\tR-0: {}\tR-1: {}\tEC50-0: {}\tEC50-1: {}\tminEC50-0: "
              "{}\tminEC50-1: {}",
              resistant_drug_pair_id, genotype->aa_sequence,
              genotype->resist_to(Model::get_drug_db()->at(drugs[0]).get()),
              genotype->resist_to(Model::get_drug_db()->at(drugs[1]).get()),
              genotype->EC50_power_n[drugs[0]], genotype->EC50_power_n[drugs[1]],
              pow(Model::get_drug_db()->at(drugs[0])->base_ec50(),
                  Model::get_drug_db()->at(drugs[0])->n()),
              pow(Model::get_drug_db()->at(drugs[1])->base_ec50(),
                  Model::get_drug_db()->at(drugs[1])->n())));
        } else {
          spdlog::debug(fmt::format(
              "resistant_drug_pair_id: {} {}\tR-0: {}\tR-1: {}\tR-2: {}\tEC50-0: {}\tEC50-1: "
              "{}\tEC50-2: {}\tminEC50-0: {}\tminEC50-1: {}\tminEC50-2: {}",
              resistant_drug_pair_id, genotype->aa_sequence,
              genotype->resist_to(Model::get_drug_db()->at(drugs[0]).get()),
              genotype->resist_to(Model::get_drug_db()->at(drugs[1]).get()),
              genotype->resist_to(Model::get_drug_db()->at(drugs[2]).get()),
              genotype->EC50_power_n[drugs[0]], genotype->EC50_power_n[drugs[1]],
              genotype->EC50_power_n[drugs[2]],
              pow(Model::get_drug_db()->at(drugs[0])->base_ec50(),
                  Model::get_drug_db()->at(drugs[0])->n()),
              pow(Model::get_drug_db()->at(drugs[1])->base_ec50(),
                  Model::get_drug_db()->at(drugs[1])->n()),
              pow(Model::get_drug_db()->at(drugs[1])->base_ec50(),
                  Model::get_drug_db()->at(drugs[2])->n())));
        }
      }
      spdlog::debug("###############");
    }
  }
}

void ValidationReporter::print_eir_pfpr_by_location(std::stringstream &ss) {
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    //
    // EIR
    if (Model::get_mdc()->eir_by_location_year()[loc].empty()) {
      ss << 0 << tsv::SEP;
      // spdlog::info("print_EIR_PfPR_by_location {}: EIR_by_location_year is empty", loc);
    } else {
      ss << Model::get_mdc()->eir_by_location_year()[loc].back() << tsv::SEP;
      // spdlog::info("print_EIR_PfPR_by_location {}: EIR_by_location_year {:.8f}", loc,
      // Model::get_mdc()->EIR_by_location_year()[loc].back());
    }
    ss << tsv::GROUP_SEP;  // 11
    // pfpr <5 , 2-10 and all
    ss << Model::get_mdc()->get_blood_slide_prevalence(loc, ModelDataCollector::PFPR_2TO10_AGE_FROM, ModelDataCollector::PFPR_2TO10_AGE_TO) * 100 << tsv::SEP;
    ss << Model::get_mdc()->get_blood_slide_prevalence(loc, ModelDataCollector::PFPR_UNDER5_AGE_FROM, ModelDataCollector::PFPR_UNDER5_AGE_TO) * 100 << tsv::SEP;
    ss << Model::get_mdc()->blood_slide_prevalence_by_location()[loc] * 100 << tsv::SEP;
  }
}
