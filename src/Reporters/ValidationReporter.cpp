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
#include "Simulation/Model.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Utility/ReporterUtils.h"
#include "Utils/Cli.h"
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
    monthly_mutation_path =
        fmt::format("{}/validation_monthly_mutation_{}.txt", path, job_number);
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

  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()){
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
  auto console_logger = spdlog::stdout_color_mt("console");
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
  std::stringstream ss;

  ss << Model::get_scheduler()->current_time() << sep;
  ss << Model::get_scheduler()->get_unix_time() << sep;
  ss << Model::get_scheduler()->get_current_date_seperated_string() << sep;
  ss << Model::get_config()->get_seasonality_settings().get_seasonal_factor(
      Model::get_scheduler()->get_calendar_date(), 0)
     << sep;
  ss << Model::get_treatment_coverage()->get_probability_to_be_treated(0, 1) << sep;
  ss << Model::get_treatment_coverage()->get_probability_to_be_treated(0, 10) << sep;
  ss << Model::get_population()->size() << sep;
  ss << group_sep;                 // 9 - index 0
  print_EIR_PfPR_by_location(ss);  // 11
  ss << group_sep;                 // 15
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_new_infections_by_location()[loc] << sep;
    ss << group_sep;  // 17
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_treatment_by_location()[loc] << sep;  // Incidence
    ss << group_sep;                                                                // 19
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[loc] << sep;
    ss << group_sep;  /// 21
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto moi : Model::get_mdc()->multiple_of_infection_by_location()[loc]) {
      ss << moi << sep;
    }
    ss << group_sep;  /// 32
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()->blood_slide_prevalence_by_location_age_group()[loc][ac] << sep;
    }
    ss << group_sep;  // 48
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->blood_slide_prevalence_by_location_age()[loc][age] << sep;
    }
    ss << group_sep;  /// 129
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->current_tf_by_location()[loc] << sep;
  }
  ss << group_sep;  // 131
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_mutation_events_by_location()[loc] << sep;
    ss << group_sep;  // 133
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()->number_of_treatments_by_location_age_year()[loc][ac] << sep;
    }
    ss << group_sep;  // 149
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->total_number_of_bites_by_location()[loc] << sep;
  }
  ss << group_sep;  // 151
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->total_number_of_bites_by_location_year()[loc] << sep;
    ss << group_sep;  // 153
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->today_number_of_treatments_by_location()[loc] << sep;
    ss << group_sep;  // 155
  }
  ss << Model::get_mdc()->current_number_of_mutation_events_in_this_year() << sep;
  ss << group_sep;  // 157
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    if ((Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
         + Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::CLINICAL])
        == 0) {
      ss << 0 << sep;
    } else {
      ss << Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
                / (Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
                   + Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::CLINICAL])
         << sep;
    }
    ss << group_sep;  // 159
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << Model::get_mdc()->popsize_by_location_age()[loc][age] << sep;
    }
    ss << group_sep;  // 240
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << Model::get_mdc()->monthly_number_of_clinical_episode_by_location_age()[loc][age] << sep;
    }
    ss << group_sep;  // 321
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      int all_infected_pop =
          Model::get_mdc()->popsize_by_location_hoststate_age_class()[loc][Person::ASYMPTOMATIC][ac]
          + Model::get_mdc()->popsize_by_location_hoststate_age_class()[loc][Person::CLINICAL][ac];
      if (all_infected_pop == 0) {
        ss << 0 << sep;
      } else {
        ss << std::setprecision(6)
           << Model::get_mdc()->number_of_clinical_by_location_age_group()[loc][ac]
                  / static_cast<double>(all_infected_pop)
           << sep;
      }
    }
    ss << group_sep;  // 337
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->total_immune_by_location()[loc]
              / static_cast<double>(Model::get_population()->size_at(loc))
       << sep;
    ss << group_sep;  // 339
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    int all_infected_pop =
        Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::ASYMPTOMATIC]
        + Model::get_mdc()->popsize_by_location_hoststate()[loc][Person::CLINICAL];
    if (all_infected_pop == 0) {
      ss << 0 << sep;
    } else {
      ss << std::setprecision(6)
         << Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[loc]
                / static_cast<double>(all_infected_pop)
         << sep;
    }
    ss << group_sep;  // 341
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_ntf_by_location()[loc] << sep;
    ss << group_sep;  // 343
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_tf_by_location()[loc] << sep;
    ss << group_sep;  // 345
  } // 417
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_number_treatments_by_location()[loc] << sep;
    ss << group_sep;  // 347
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_tf_by_location()[loc] << sep;
    ss << group_sep;  // 349
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_clinical_episodes_by_location()[loc] << sep;
    ss << group_sep;  // 351
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_untreated_cases_by_location_age_year()[loc][age] << sep;
    }
    ss << group_sep;  /// 432
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_deaths_by_location_age_year()[loc][age] << sep;
    }
    ss << group_sep;  /// 513
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_malaria_deaths_treated_by_location_age_year()[loc][age]
         << sep;
    }
    ss << group_sep;  /// 594
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->number_of_malaria_deaths_non_treated_by_location_age_year()[loc][age]
         << sep;
    }
    ss << group_sep;  /// 675
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->total_immune_by_location_age()[loc][age]
         << sep;
    }
    ss << group_sep;  /// 756
  }
  for (auto tf_by_therapy : Model::get_mdc()->current_tf_by_therapy()) {
    ss << tf_by_therapy << sep;
  }
  ss << group_sep;  // 772
  for (int t_id = 0; t_id < Model::get_therapy_db().size(); t_id++) {
    int n_treaments = Model::get_mdc()->number_of_treatments_with_therapy_id()[t_id];
    int n_success = Model::get_mdc()->number_of_treatments_success_with_therapy_id()[t_id];
    int n_fail = Model::get_mdc()->number_of_treatments_fail_with_therapy_id()[t_id];
    double p_success = (n_treaments == 0) ? 0 : n_success * 100.0 / n_treaments;
    ss << n_treaments << sep << n_success << sep << n_fail << sep << p_success << sep;
  }
  ss << group_sep; // 833
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << Model::get_mdc()->cumulative_clinical_episodes_by_location_age()[loc][age]
         << sep;
    }
    ss << group_sep;  // 914
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->progress_to_clinical_in_7d_counter[loc].total
         << sep;
    ss << group_sep;  // 916
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->progress_to_clinical_in_7d_counter[loc].recrudescence
         << sep;
    ss << group_sep;  // 918
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->progress_to_clinical_in_7d_counter[loc].new_infection
         << sep;
    ss << group_sep;  // 920
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_number_of_recrudescence_treatment_by_location()[loc]
         << sep;
    ss << group_sep;  // 922
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()->monthly_number_of_recrudescence_treatment_by_location_age_class()[loc][ac]
         << sep;
    }
    ss << group_sep;  /// 938
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int age = 0; age < 80; age++) {
      ss << Model::get_mdc()->monthly_number_of_recrudescence_treatment_by_location_age()[loc][age]
         << sep;
    }
    ss << group_sep;  /// 1019
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->monthly_treatment_failure_by_location()[loc]
         << sep;
    ss << group_sep;  // 1021
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      ss << Model::get_mdc()->monthly_treatment_failure_by_location_age_class()[loc][ac]
        << sep;
    }
    ss << group_sep;  // 1107
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

  if (Model::get_config()->get_mosquito_parameters().get_record_recombination_events()){
    int sum = 0;
    for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
      sum += static_cast<int>(Model::get_mdc()->mutation_tracker[loc].size());
      for (auto &yearly_data : Model::get_mdc()->mutation_tracker[loc]) {
        ss << std::get<0>(yearly_data) << sep;
        ss << std::get<1>(yearly_data) << sep;
        ss << std::get<2>(yearly_data) << sep;
        ss << std::get<3>(yearly_data) << sep;
        ss << std::get<4>(yearly_data) << sep;
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
        ss << std::get<0>(genotype_tracked) << sep;
        ss << std::get<1>(genotype_tracked) << sep;
        ss << std::get<2>(genotype_tracked) << sep;
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
  ss << Model::get_random()->get_seed() << sep << Model::get_config()->number_of_locations() << sep;
  ss << Model::get_config()->location_db()[0].beta << sep;
  ss << Model::get_config()->location_db()[0].population_size << sep;
  print_EIR_PfPR_by_location(ss);
  ss << group_sep;  // 9
  // output last strategy information
  ss << Model::get_treatment_strategy()->id << sep;  // 10
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
  ss << (sum_ntf * 100 / static_cast<double>(pop_size)) / total_time_in_years << sep;
  ss << group_sep;  // 12
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_clinical_episodes_by_location()[loc] << sep;
    ss << group_sep;  // 14
  }
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->percentage_bites_on_top_20_by_location()[loc] * 100 << "%" << sep;
    //        std::cout << Model::get_mdc()->percentage_bites_on_top_20_by_location()[loc] * 100 <<
    //        "%" << "\t";
    ss << group_sep;  // 16
  }
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    double location_ntf = Model::get_mdc()->cumulative_ntf_by_location()[loc] * 100
                          / static_cast<double>(Model::get_mdc()->popsize_by_location()[loc]);
    location_ntf /= total_time_in_years;
    ss << location_ntf << sep;
    //        std::cout << location_NTF << "\t";
    ss << group_sep;  // 18
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto age = 0; age < 80; age++) {
      ss << static_cast<double>(
                Model::get_mdc()->cumulative_clinical_episodes_by_location_age()[loc][age])
                / total_time_in_years / Model::get_mdc()->popsize_by_location_age()[loc][age]
         << sep;
    }
    ss << group_sep;  // 99
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_number_treatments_by_location()[loc] << sep;
    ss << group_sep;  // 101
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_tf_by_location()[loc] << sep;
    ss << group_sep;  // 103
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->cumulative_clinical_episodes_by_location()[loc] << sep;
    ss << group_sep;  // 105
  }
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    ss << Model::get_mdc()->mosquito_recombination_events_count()[loc][0] << sep;
    ss << Model::get_mdc()->mosquito_recombination_events_count()[loc][1] << sep;
    ss << group_sep;  // 107
  }
  summary_data_logger->info(ss.str());

  for (const auto &genotype : *(Model::get_genotype_db())) {
    gene_db_logger->info("{}{}{}", genotype->genotype_id(), sep, genotype->aa_sequence);
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
              pow(Model::get_drug_db()->at(drugs[0])->base_EC50,
                  Model::get_drug_db()->at(drugs[0])->n()),
              pow(Model::get_drug_db()->at(drugs[1])->base_EC50,
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
              pow(Model::get_drug_db()->at(drugs[0])->base_EC50,
                  Model::get_drug_db()->at(drugs[0])->n()),
              pow(Model::get_drug_db()->at(drugs[1])->base_EC50,
                  Model::get_drug_db()->at(drugs[1])->n()),
              pow(Model::get_drug_db()->at(drugs[1])->base_EC50,
                  Model::get_drug_db()->at(drugs[2])->n())));
        }
      }
      spdlog::debug("###############");
    }
  }
}

void ValidationReporter::print_EIR_PfPR_by_location(std::stringstream &ss) {
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    //
    // EIR
    if (Model::get_mdc()->eir_by_location_year()[loc].empty()) {
      ss << 0 << sep;
      // spdlog::info("print_EIR_PfPR_by_location {}: EIR_by_location_year is empty", loc);
    } else {
      ss << Model::get_mdc()->eir_by_location_year()[loc].back() << sep;
      // spdlog::info("print_EIR_PfPR_by_location {}: EIR_by_location_year {:.8f}", loc,
      // Model::get_mdc()->EIR_by_location_year()[loc].back());
    }
    ss << group_sep;  // 11
    // pfpr <5 , 2-10 and all
    ss << Model::get_mdc()->get_blood_slide_prevalence(loc, 2, 10) * 100 << sep;
    ss << Model::get_mdc()->get_blood_slide_prevalence(loc, 0, 5) * 100 << sep;
    ss << Model::get_mdc()->blood_slide_prevalence_by_location()[loc] * 100 << sep;
  }
}

