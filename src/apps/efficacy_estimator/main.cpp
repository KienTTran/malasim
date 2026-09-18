/*
 * main.cpp
 *
 * Alternative executable built against the malaria simulation that can be used
 * to calculate the drug efficacies for the various genotypes.
 *
 * Includes recurrence_test mode ported from TMS.
 */

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Configuration/Config.h"
#include "apps/efficacy_estimator/DxGCli.h"
#include "Events/ProgressToClinicalEvent.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "PkPdReporter.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Treatment/Strategies/IStrategy.h"
#include "Treatment/Strategies/SFTStrategy.h"
#include "Treatment/Therapies/SCTherapy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Utils/Index/PersonIndexAll.h"
#include "Utils/Random.h"
#include "apps/malasim/MaSimAppInput.h"

double get_efficacy_for_therapy(std::string g_str,
                                Model* p_model,
                                utils::DxGAppInput &input,
                                int therapy_id);
double get_efficacy_for_therapy_crt(Model* p_model, utils::DxGAppInput &input, int therapy_id);
double get_efficacy_for_therapy_recurrence_test(std::string g_str,
                                                Model* p_model,
                                                utils::DxGAppInput &input,
                                                int therapy_id);

namespace {

using DxGAppInput = utils::DxGAppInput;

void initialize_model(const DxGAppInput &input) {
  utils::MaSimAppInput cli_input{};
  cli_input.input_path = input.input_file;
  Model::set_cli_input(std::move(cli_input));
  Model::get_instance()->initialize();
}

void apply_drug_overrides(const DxGAppInput &input) {
  if (input.as_iiv != -1) {
    for (auto &sd : Model::get_drug_db()->at(0)->age_group_specific_drug_concentration_sd()) {
      sd = input.as_iiv;
    }
  }

  if (input.as_ec50 != -1) {
    // TODO: Check this
    Model::get_drug_db()->at(0)->set_base_ec50(input.as_ec50);
  }
}

std::vector<int> get_therapy_ids(const DxGAppInput &input) {
  if (!input.therapy_list.empty()) { return input.therapy_list; }

  int min_therapy_id = 0;
  int max_therapy_id = 0;
  if (input.therapies.size() == 1) {
    min_therapy_id = input.therapies.front();
    max_therapy_id = input.therapies.front();
  } else if (input.therapies.size() == 2) {
    min_therapy_id = input.therapies.front();
    max_therapy_id = input.therapies.back();
  }

  std::vector<int> therapy_ids;
  for (auto therapy_id = min_therapy_id; therapy_id <= max_therapy_id; ++therapy_id) {
    therapy_ids.push_back(therapy_id);
  }
  return therapy_ids;
}

void print_therapy_header(const std::vector<int> &therapy_ids, const char* separator) {
  for (const auto therapy_id : therapy_ids) {
    std::cout << *Model::get_therapy_db()[therapy_id] << separator;
  }
  std::cout << '\n';
}

void run_recurrence_test(Model* model, DxGAppInput &input, const std::vector<int> &therapy_ids) {
  if (input.genotypes.empty()) {
    std::cout << "List of genotypes is empty for recurrence test" << '\n';
    return;
  }

  std::cout << "ID,Genotype";
  for (const auto therapy_id : therapy_ids) {
    std::cout << "," << *Model::get_therapy_db()[therapy_id];
  }
  std::cout << '\n';

  for (std::size_t genotype_index = 0; genotype_index < input.genotypes.size(); ++genotype_index) {
    std::stringstream output;
    output << genotype_index << "," << input.genotypes[genotype_index];
    for (const auto therapy_id : therapy_ids) {
      const auto efficacy = get_efficacy_for_therapy_recurrence_test(
          input.genotypes[genotype_index], model, input, therapy_id);
      output << "," << efficacy;
    }
    std::cout << output.str() << '\n';
  }
}

void run_crt_calibration(Model* model, DxGAppInput &input, const std::vector<int> &therapy_ids) {
  if (input.genotypes.empty()) {
    std::cout << "List of population genotypes is empty" << '\n';
    return;
  }

  print_therapy_header(therapy_ids, "\t");
  std::stringstream output;
  for (std::size_t therapy_index = 0; therapy_index < therapy_ids.size(); ++therapy_index) {
    output << get_efficacy_for_therapy_crt(model, input, therapy_ids[therapy_index]);
    if (therapy_index + 1 < therapy_ids.size()) { output << '\t'; }
  }
  std::cout << output.str() << '\n';
}

void resize_population_if_needed(const DxGAppInput &input) {
  if (input.population_size == Model::get_population()->size()) { return; }

  Model::get_config()->location_db()[0].population_size = input.population_size;
  Model::set_population(std::make_unique<Population>());
  Model::get_population()->initialize();
}

void configure_ee_drugs(const DxGAppInput &input) {
  const auto start_drug_id = input.is_art ? 0 : 1;
  for (int i = 0; i < input.number_of_drugs_in_combination; ++i) {
    auto* drug_type = Model::get_drug_db()->at(i + start_drug_id).get();
    drug_type->set_name(fmt::format("D{}", i));
    drug_type->set_drug_half_life(input.half_life[i]);
    drug_type->set_maximum_parasite_killing_rate(input.k_max[i]);
    drug_type->set_n(input.slope[i]);
    drug_type->set_k(4);
    for (auto &mean_drug_absorption : drug_type->age_specific_drug_absorption()) {
      mean_drug_absorption = input.mean_drug_absorption[i];
    }
  }

  auto* therapy = dynamic_cast<SCTherapy*>(Model::get_therapy_db()[0].get());
  therapy->drug_ids.clear();
  therapy->dosing_day.clear();
  for (int i = 0; i < input.number_of_drugs_in_combination; ++i) {
    therapy->drug_ids.push_back(i + start_drug_id);
    therapy->dosing_day.push_back(input.dosing_days[i]);
  }
}

Genotype* get_ee_genotype(const DxGAppInput &input) {
  Model::get_genotype_db()->clear();
  auto* input_genotype = Model::get_genotype_db()->get_genotype(input.genotypes.front());
  return Model::get_genotype_db()->get_genotype(input_genotype->get_aa_sequence());
}

void infect_population_for_ee(Genotype* genotype) {
  for (auto &person : Model::get_population()->all_persons()->v_person()) {
    const auto density = Model::get_config()
                             ->get_parasite_parameters()
                             .get_parasite_density_levels()
                             .get_log_parasite_density_from_liver();
    auto* blood_parasite = person->add_new_parasite_to_blood(genotype);

    person->get_immune_system()->set_increase(true);
    person->set_host_state(Person::EXPOSED);
    blood_parasite->set_gametocyte_level(
        Model::get_config()->get_epidemiological_parameters().get_gametocyte_level_full());
    blood_parasite->set_last_update_log10_parasite_density(density);

    const auto &epidemiological_parameters = Model::get_config()->get_epidemiological_parameters();
    const int days_to_clinical = person->get_age() <= 5
                                     ? epidemiological_parameters.get_days_to_clinical_under_five()
                                     : epidemiological_parameters.get_days_to_clinical_over_five();
    auto event = std::make_unique<ProgressToClinicalEvent>(person.get());
    event->set_time(Person::calculate_future_time(days_to_clinical));
    event->set_clinical_caused_parasite(blood_parasite);
    person->schedule_basic_event(std::move(event));
  }
}

void run_ee_calibration(Model* model, DxGAppInput &input) {
  resize_population_if_needed(input);
  configure_ee_drugs(input);
  model->get_reporters().clear();
  model->add_reporter(std::make_unique<PkPdReporter>(&input));
  infect_population_for_ee(get_ee_genotype(input));
  model->run();
}

void run_default_mode(Model* model, DxGAppInput &input, const std::vector<int> &therapy_ids) {
  std::cout << "ID\tGenotype\t";
  print_therapy_header(therapy_ids, "\t");

  for (std::size_t genotype_index = 0; genotype_index < input.genotypes.size(); ++genotype_index) {
    std::stringstream output;
    const auto &genotype = input.genotypes[genotype_index];
    const auto genotype_label = input.is_old_format ? Mosquito::get_old_genotype_string2(genotype)
                                                    : Mosquito::get_old_genotype_string(genotype);
    output << genotype_index << '\t' << genotype_label << '\t';
    for (const auto therapy_id : therapy_ids) {
      output << get_efficacy_for_therapy(genotype, model, input, therapy_id) << '\t';
    }
    std::cout << output.str() << '\n';
  }
}

}  // namespace

inline double round(double val) {
  if (val < 0) return ceil(val - 0.5);
  return floor(val + 0.5);
}

int main(int argc, char** argv) {
  auto input = utils::cli::parse<utils::DxGCli>(argc, argv);
  if (!utils::cli::validate<utils::DxGCli>(input)) { return 1; }
  initialize_model(input);
  auto* model = Model::get_instance();
  apply_drug_overrides(input);
  std::cout << std::setprecision(5);
  const auto therapy_ids = get_therapy_ids(input);

  if (input.is_recurrence_test) {
    run_recurrence_test(model, input, therapy_ids);
  } else if (input.is_crt_calibration) {
    run_crt_calibration(model, input, therapy_ids);
  } else if (input.is_ee_calibration) {
    run_ee_calibration(model, input);
  } else {
    run_default_mode(model, input, therapy_ids);
  }

  return 0;
}

// ==================== RECURRENCE TEST (TMS logic in malasim API) ====================
double get_efficacy_for_therapy_recurrence_test(std::string g_str,
                                                Model* p_model,
                                                utils::DxGAppInput &input,
                                                int therapy_id) {
  Therapy* main_therapy = Model::get_therapy_db()[therapy_id].get();

  // Verify and set the strategy
  auto* strategy = dynamic_cast<SFTStrategy*>(Model::get_treatment_strategy());
  if (strategy == nullptr) {
    std::cerr << "The recurrence_test mode can only be used with a SFTStrategy!" << '\n';
    exit(EXIT_FAILURE);
  }
  strategy->get_therapy_list().clear();
  strategy->add_therapy(main_therapy);

  // Reset reporters and add PkPdReporter with CSV per-person output
  p_model->get_reporters().clear();

  auto pkpd_reporter = std::make_unique<PkPdReporter>(&input);
  auto* pkpd_ptr = pkpd_reporter.get();
  p_model->add_reporter(std::move(pkpd_reporter));

  // Initialize with prefix for per-genotype/therapy output file
  std::string path_prefix = fmt::format("{}_{}", therapy_id, g_str);
  pkpd_ptr->initialize(0, path_prefix);

  // Infect all persons with the specified genotype
  auto* genotype = Model::get_genotype_db()->get_genotype(g_str);

  for (auto &person : Model::get_population()->all_persons()->v_person()) {
    person->get_all_clonal_parasite_populations()->clear();
    auto density = Model::get_config()
                       ->get_parasite_parameters()
                       .get_parasite_density_levels()
                       .get_log_parasite_density_from_liver();
    auto* blood_parasite = person->add_new_parasite_to_blood(genotype);

    person->get_immune_system()->set_increase(true);
    person->set_host_state(Person::EXPOSED);

    blood_parasite->set_gametocyte_level(
        Model::get_config()->get_epidemiological_parameters().get_gametocyte_level_full());
    blood_parasite->set_last_update_log10_parasite_density(density);

    auto event = std::make_unique<ProgressToClinicalEvent>(person.get());
    event->set_time(0);
    event->set_clinical_caused_parasite(blood_parasite);
    person->schedule_basic_event(std::move(event));
  }

  p_model->run();

  // TMS uses treatment failure rate instead of blood slide prevalence
  const auto result = 1
                      - (1.0 * Model::get_mdc()->monthly_treatment_failure_by_location()[0]
                         / static_cast<double>(Model::get_population()->size()));

  // Reset simulation state for next run
  Model::set_population(std::make_unique<Population>());
  Model::set_scheduler(std::make_unique<Scheduler>());

  Model::get_scheduler()->initialize(
      Model::get_config()->get_simulation_timeframe().get_starting_date(),
      Model::get_config()->get_simulation_timeframe().get_ending_date());
  Model::get_population()->initialize();

  return result;
}

// ==================== EXISTING: default efficacy ====================
double get_efficacy_for_therapy(std::string g_str,
                                Model* p_model,
                                utils::DxGAppInput &input,
                                int therapy_id) {
  Therapy* main_therapy = Model::get_therapy_db()[therapy_id].get();
  dynamic_cast<SFTStrategy*>(Model::get_treatment_strategy())->get_therapy_list().clear();
  dynamic_cast<SFTStrategy*>(Model::get_treatment_strategy())->add_therapy(main_therapy);

  p_model->get_reporters().clear();
  if (!input.output_file.empty()) {
    p_model->add_reporter(std::make_unique<PkPdReporter>(&input));
  } else {
    p_model->add_reporter(std::make_unique<PkPdReporter>());
  }

  for (auto &person : Model::get_population()->all_persons()->v_person()) {
    auto density = Model::get_config()
                       ->get_parasite_parameters()
                       .get_parasite_density_levels()
                       .get_log_parasite_density_from_liver();
    auto* genotype = Model::get_genotype_db()->get_genotype(g_str);
    auto* blood_parasite = person->add_new_parasite_to_blood(genotype);

    person->get_immune_system()->set_increase(true);
    person->set_host_state(Person::EXPOSED);

    blood_parasite->set_gametocyte_level(
        Model::get_config()->get_epidemiological_parameters().get_gametocyte_level_full());
    blood_parasite->set_last_update_log10_parasite_density(density);

    const int days_to_clinical = (person->get_age() <= 5) ? Model::get_config()
                                                                ->get_epidemiological_parameters()
                                                                .get_days_to_clinical_under_five()
                                                          : Model::get_config()
                                                                ->get_epidemiological_parameters()
                                                                .get_days_to_clinical_over_five();
    auto event = std::make_unique<ProgressToClinicalEvent>(person.get());
    event->set_time(Person::calculate_future_time(days_to_clinical));
    event->set_clinical_caused_parasite(blood_parasite);
    person->schedule_basic_event(std::move(event));
  }

  p_model->run();
  const auto result = 1 - Model::get_mdc()->blood_slide_prevalence_by_location()[0];

  Model::set_population(std::make_unique<Population>());
  Model::set_scheduler(std::make_unique<Scheduler>());
  Model::get_scheduler()->initialize(
      Model::get_config()->get_simulation_timeframe().get_starting_date(),
      Model::get_config()->get_simulation_timeframe().get_ending_date());
  Model::get_population()->initialize();

  return result;
}

// ==================== EXISTING: CRT calibration ====================
double get_efficacy_for_therapy_crt(Model* p_model, utils::DxGAppInput &input, int therapy_id) {
  Therapy* main_therapy = Model::get_therapy_db()[therapy_id].get();
  dynamic_cast<SFTStrategy*>(Model::get_treatment_coverage())->get_therapy_list().clear();
  dynamic_cast<SFTStrategy*>(Model::get_treatment_coverage())->add_therapy(main_therapy);

  p_model->get_reporters().clear();
  if (!input.output_file.empty()) {
    p_model->add_reporter(std::make_unique<PkPdReporter>(&input));
  } else {
    p_model->add_reporter(std::make_unique<PkPdReporter>());
  }

  for (auto &person : Model::get_population()->all_persons()->v_person()) {
    std::string g_str;
    int infect_prob = Model::get_random()->random_uniform(1, 104);
    if (infect_prob < 74) {
      g_str = input.genotypes[2];
    } else if (infect_prob < 91) {
      g_str = input.genotypes[1];
    } else {
      g_str = input.genotypes[0];
    }
    auto* genotype = Model::get_genotype_db()->get_genotype(g_str);
    auto* blood_parasite = person->add_new_parasite_to_blood(genotype);
    auto density = Model::get_config()
                       ->get_parasite_parameters()
                       .get_parasite_density_levels()
                       .get_log_parasite_density_from_liver();

    person->get_immune_system()->set_increase(true);
    person->set_host_state(Person::EXPOSED);

    blood_parasite->set_gametocyte_level(
        Model::get_config()->get_epidemiological_parameters().get_gametocyte_level_full());
    blood_parasite->set_last_update_log10_parasite_density(density);

    const int days_to_clinical = (person->get_age() <= 5) ? Model::get_config()
                                                                ->get_epidemiological_parameters()
                                                                .get_days_to_clinical_under_five()
                                                          : Model::get_config()
                                                                ->get_epidemiological_parameters()
                                                                .get_days_to_clinical_over_five();
    auto event = std::make_unique<ProgressToClinicalEvent>(person.get());
    event->set_time(Person::calculate_future_time(days_to_clinical));
    event->set_clinical_caused_parasite(blood_parasite);
    person->schedule_basic_event(std::move(event));
  }

  p_model->run();
  const auto result = 1 - Model::get_mdc()->blood_slide_prevalence_by_location()[0];

  Model::set_population(std::make_unique<Population>());
  Model::set_scheduler(std::make_unique<Scheduler>());
  Model::get_scheduler()->initialize(
      Model::get_config()->get_simulation_timeframe().get_starting_date(),
      Model::get_config()->get_simulation_timeframe().get_ending_date());
  Model::get_population()->initialize();

  return result;
}
