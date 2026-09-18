#include "Population.h"

#include <Simulation/Model.h>
#include <Utils/Random.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cfloat>
#include <memory>
#include <stdexcept>

#include "ClinicalUpdateFunction.h"
#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Events/BirthdayEvent.h"
#include "Events/SwitchImmuneSystemModeEvent.h"
#include "ImmuneSystem/ImmuneSystem.h"
#include "MDC/ModelDataCollector.h"
#include "Mosquito/Mosquito.h"
#include "Parasites/Genotype.h"
#include "Person/Person.h"
#include "Utils/Constants.h"
#include "Utils/Index/PersonIndex.h"
#include "Utils/Index/PersonIndexAll.h"
#include "Utils/Index/PersonIndexByLocationMovingLevel.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"

Population::Population() {
  person_index_list_ = std::make_unique<PersonIndexPtrList>();

  all_persons_ = std::make_unique<PersonIndexAll>();
}

Population::~Population() {
  // persons_.clear();
  // release memory for all persons
  if (all_persons_ != nullptr) {
    all_persons_->clear();
    all_persons_.reset();
  }

  // release person_indexes
  if (person_index_list_ != nullptr) {
    person_index_list_->clear();
    person_index_list_.reset();
  }
}

void Population::initialize() {
  if (Model::get_instance() != nullptr) {
    all_persons_->clear();
    // those vector will be used in the initial infection
    const auto number_of_locations = Model::get_config()->number_of_locations();

    // Prepare the population size vector
    popsize_by_location_ = IntVector(number_of_locations, 0);

    individual_relative_biting_by_location_ =
        std::vector<std::vector<double>>(number_of_locations, std::vector<double>());
    individual_relative_moving_by_location_ =
        std::vector<std::vector<double>>(number_of_locations, std::vector<double>());
    individual_foi_by_location_ =
        std::vector<std::vector<double>>(number_of_locations, std::vector<double>());

    all_alive_persons_by_location_ =
        std::vector<std::vector<Person*>>(number_of_locations, std::vector<Person*>());

    sum_relative_biting_by_location_ = std::vector<double>(number_of_locations, 0);
    sum_relative_moving_by_location_ = std::vector<double>(number_of_locations, 0);

    current_force_of_infection_by_location_ = std::vector<double>(number_of_locations, 0);

    force_of_infection_for_n_days_by_location_ =
        std::vector<std::vector<double>>(Model::get_config()->number_of_tracking_days(),
                                         std::vector<double>(number_of_locations, 0));

    // initalize person indexes
    initialize_person_indices();

    // Initialize population
    auto &location_db = Model::get_config()->location_db();
    for (auto loc = 0; loc < number_of_locations; loc++) {
      const auto popsize_by_location =
          static_cast<int>(location_db[loc].population_size
                           * Model::get_config()
                                 ->get_population_demographic()
                                 .get_artificial_rescaling_of_population_size());
      const auto location_capacity =
          popsize_by_location > 0 ? static_cast<std::size_t>(popsize_by_location) : std::size_t{0};
      individual_relative_biting_by_location_[loc].reserve(location_capacity);
      individual_relative_moving_by_location_[loc].reserve(location_capacity);
      individual_foi_by_location_[loc].reserve(location_capacity);
      all_alive_persons_by_location_[loc].reserve(location_capacity);
      auto temp_sum = 0;
      for (auto age_class = 0;
           age_class
           < Model::get_config()->get_population_demographic().get_initial_age_structure().size();
           age_class++) {
        auto number_of_individual_by_loc_ageclass = 0;
        if (age_class
            == Model::get_config()->get_population_demographic().get_initial_age_structure().size()
                   - 1) {
          number_of_individual_by_loc_ageclass = popsize_by_location - temp_sum;
        } else {
          number_of_individual_by_loc_ageclass =
              static_cast<int>(popsize_by_location * location_db[loc].age_distribution[age_class]);
          temp_sum += number_of_individual_by_loc_ageclass;
        }
        // spdlog::info("generate {} individuals for age class {}",
        //              number_of_individual_by_loc_ageclass, age_class);
        for (int i = 0; i < number_of_individual_by_loc_ageclass; i++) {
          generate_individual(loc, age_class);
        }
      }
      popsize_by_location_[loc] = popsize_by_location;
      // spdlog::info("individual_relative_moving_by_location[{}] size: {} sum: {}",loc,
      //              individual_relative_moving_by_location[loc].size(),
      //              sum_relative_moving_by_location[loc]);
    }
  }
}

void Population::initialize_person_indices() {
  const int number_of_location = static_cast<int>(Model::get_config()->number_of_locations());
  const int number_of_host_states = Person::NUMBER_OF_STATE;
  const int number_of_age_classes = Model::get_config()->number_of_age_classes();

  auto p_index_by_l_s_a = std::make_unique<PersonIndexByLocationStateAgeClass>(
      number_of_location, number_of_host_states, number_of_age_classes);
  person_index_list_->push_back(std::move(p_index_by_l_s_a));

  auto p_index_location_moving_level = std::make_unique<PersonIndexByLocationMovingLevel>(
      number_of_location, Model::get_config()
                              ->get_movement_settings()
                              .get_circulation_info()
                              .get_number_of_moving_levels());
  person_index_list_->push_back(std::move(p_index_location_moving_level));
}

void Population::add_person(std::unique_ptr<Person> person) {
  // persons_.push_back(person);
  person->set_population(this);
  for (auto &person_index : *person_index_list_) { person_index->add(person.get()); }

  // Update the count at the location
  if (person->get_location() != core::K_INVALID_LOCATION_ID) {
    popsize_by_location_[person->get_location()]++;
  }
  // all_persons will take ownership
  all_persons_->add(std::move(person));
}

void Population::remove_dead_person(Person* person) { remove_person(person); }

void Population::remove_person(Person* person) {
  // persons_.erase(std::ranges::remove(persons_, person).begin(), persons_.end());
  if (person->get_location() != core::K_INVALID_LOCATION_ID) {
    popsize_by_location_[person->get_location()]--;
  }
  for (auto &person_index : *person_index_list_) { person_index->remove(person); }
  all_persons_->remove(person);
}

void Population::notify_change(Person* person,
                               const Person::Property &property,
                               const void* old_value,
                               const void* new_value) {
  for (auto &person_index : *person_index_list_) {
    person_index->notify_change(person, property, old_value, new_value);
  }
}

void Population::notify_movement(const int source, const int destination) {
  popsize_by_location_[source]--;
  assert(popsize_by_location_[source] >= 0);
  popsize_by_location_[destination]++;
}

std::size_t Population::size(const int &location, const int &age_class) {
  if (location == -1) { return all_persons_->size(); }
  auto* pi_lsa = get_person_index<PersonIndexByLocationStateAgeClass>();

  if (pi_lsa == nullptr) { return 0; }
  std::size_t temp = 0;
  if (age_class == core::K_INVALID_AGE_CLASS) {
    for (auto state = 0; state < Person::NUMBER_OF_STATE - 1; state++) {
      for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        temp += pi_lsa->vPerson()[location][state][ac].size();
      }
    }
  } else {
    for (auto state = 0; state < Person::NUMBER_OF_STATE - 1; state++) {
      temp += pi_lsa->vPerson()[location][state][age_class].size();
    }
  }
  return temp;
}

std::size_t Population::size(const int &location,
                             const Person::HostStates &hs,
                             const int &age_class) {
  if (location == -1) { return all_persons_->size(); }
  auto* pi_lsa = get_person_index<PersonIndexByLocationStateAgeClass>();
  return (pi_lsa->vPerson()[location][hs][age_class].size());
}

// new
std::size_t Population::size_residents_only(const int &location) {
  if (location == -1) { return all_persons_->size(); }

  auto* pi_lsa = get_person_index<PersonIndexByLocationStateAgeClass>();

  if (pi_lsa == nullptr) { return 0; }
  auto temp = 0;
  for (auto state = 0; state < Person::NUMBER_OF_STATE - 1; state++) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      for (auto i = 0; i < pi_lsa->vPerson()[location][state][ac].size(); i++) {
        if (pi_lsa->vPerson()[location][state][ac][i]->get_residence_location() == location) {
          temp++;
        }
      }
    }
  }
  return temp;
}

// std::size_t Population::size() { return persons_.size(); }

std::size_t Population::size_at(const int &location) { return popsize_by_location_[location]; }

void Population::perform_infection_event_at_location(const int location, const int tracking_index) {
  PersonPtrVector infected_people;
  const bool use_sporozoite_challenge =
      Model::get_config()->get_transmission_settings().get_transmission_parameter() > 0.0;
  process_infections_at_location(location, tracking_index, use_sporozoite_challenge,
                                 infected_people);
  finalize_today_infections(infected_people);
}

double Population::calculate_challenge_infection_probability(
    Person &person, const double transmission_probability) {
  const double immunity = person.get_immune_system()->get_current_value();
  if (immunity <= 0.2) { return transmission_probability; }
  if (immunity >= 0.8) { return 0.1; }

  // Transition smoothly from the configured risk at low immunity to the
  // challenge-mode floor at high immunity.
  const double normalized_immunity = (immunity - 0.2) / 0.6;
  return (transmission_probability * (1.0 - normalized_immunity)) + (0.1 * normalized_immunity);
}

bool Population::is_infection_caused_by_bite(Person &person, const bool use_sporozoite_challenge) {
  auto* config = Model::get_config();
  const double draw = Model::get_random()->random_flat(0.0, 1.0);

  if (use_sporozoite_challenge) {
    const double transmission_probability =
        config->get_transmission_settings().get_transmission_parameter();
    return draw < calculate_challenge_infection_probability(person, transmission_probability);
  }

  const auto &epidemiological_parameters = config->get_epidemiological_parameters();
  const double infection_probability =
      epidemiological_parameters.get_using_variable_probability_infectious_bites_cause_infection()
          ? person.p_infection_from_an_infectious_bite()
          : config->get_transmission_settings().get_p_infection_from_an_infectious_bite();
  return draw <= infection_probability;
}

bool Population::can_receive_new_infection(Person &person) {
  return person.get_host_state() != Person::EXPOSED && person.liver_parasite_type() == nullptr;
}

void Population::process_bite(Person &person,
                              const int location,
                              const int tracking_index,
                              const bool use_sporozoite_challenge,
                              PersonPtrVector &infected_people) {
  assert(person.get_host_state() != Person::DEAD);

  // A bite is exposure history even when no genotype is present or infection
  // fails. This keeps bite accounting identical in both transmission modes.
  person.increase_number_of_times_bitten();

  const int genotype_id = Model::get_mosquito()->random_genotype(location, tracking_index);
  if (genotype_id < 0 || !is_infection_caused_by_bite(person, use_sporozoite_challenge)
      || !can_receive_new_infection(person)) {
    return;
  }

  // Repeated sampling can give one person several successful bites. Retain all
  // candidate genotypes but enqueue the person for finalization only once.
  const bool first_infection_today = person.get_today_infections().empty();
  person.get_today_infections().push_back(genotype_id);
  if (first_infection_today) { infected_people.push_back(&person); }
}

void Population::process_infections_at_location(const int location,
                                                const int tracking_index,
                                                const bool use_sporozoite_challenge,
                                                PersonPtrVector &infected_people) {
  const double force_of_infection =
      force_of_infection_for_n_days_by_location_[tracking_index][location];
  if (force_of_infection <= DBL_EPSILON) { return; }

  auto &alive_people = all_alive_persons_by_location_[location];
  if (alive_people.empty()) {
    spdlog::trace("all_alive_persons_by_location location {} is empty", location);
    return;
  }

  const double total_relative_biting = sum_relative_biting_by_location_[location];
  if (total_relative_biting <= 0.0) {
    spdlog::trace("sum_relative_biting_by_location[{}] is zero", location);
    return;
  }

  if (Model::get_mosquito()->genotypes_table[tracking_index][location].empty()) {
    spdlog::trace("mosquito genotypes_table[{}][{}] is empty", tracking_index, location);
    return;
  }

  auto* config = Model::get_config();
  const double seasonal_factor = config->get_seasonality_settings().get_seasonal_factor(
      Model::get_scheduler()->get_calendar_date(), location);
  const double poisson_mean =
      config->location_db()[location].beta * seasonal_factor * force_of_infection;
  const int number_of_bites = Model::get_random()->random_poisson(poisson_mean);
  if (number_of_bites <= 0) { return; }

  // Roulette sampling is with replacement, so number_of_bites may exceed the
  // number of people and the same person may be selected more than once. The
  // boolean argument controls result shuffling only.
  auto bitten_people = Model::get_random()->roulette_sampling<Person>(
      number_of_bites, individual_relative_biting_by_location_[location], alive_people,
      /*is_shuffled=*/false, total_relative_biting);

  // Record only bites that reached recipient sampling; invalid location state
  // and missing mosquito cohorts have already returned above.
  Model::get_mdc()->collect_number_of_bites(location, static_cast<int>(bitten_people.size()));
  for (auto* person : bitten_people) {
    process_bite(*person, location, tracking_index, use_sporozoite_challenge, infected_people);
  }
}

void Population::finalize_today_infections(const PersonPtrVector &infected_people) {
  for (auto* person : infected_people) {
    Model::get_mdc()->record_1_infection(person->get_location());
    // Select one genotype when multiple successful bites occurred, create the
    // liver-stage infection, and clear the person's candidates for tomorrow.
    person->randomly_choose_parasite();
  }
}

void Population::generate_individual(int location, int age_class) {
  auto* config = Model::get_config();
  auto* random = Model::get_random();
  auto* scheduler = Model::get_scheduler();

  auto person = std::make_unique<Person>();
  person->initialize();

  person->set_location(location);
  person->set_residence_location(location);
  person->set_host_state(Person::SUSCEPTIBLE);

  auto initial_age_structure = config->get_population_demographic().get_initial_age_structure();
  // Set the age of the individual, which also sets the age class. Note that we
  // are defining the types to conform to the signature of random_uniform<int>
  const int age_from = (age_class == 0) ? 0 : initial_age_structure[age_class - 1];
  const int age_to = initial_age_structure[age_class];
  person->set_age(random->random_uniform<int>(age_from, age_to + 1));  // Upper bound is exclusive.

  auto days_to_next_birthday =
      static_cast<int>(Model::get_random()->random_uniform((Constants::DAYS_IN_YEAR))) + 1;

  // this will get the birthday in simulation time
  auto ymd = Model::get_scheduler()->get_ymd_after_days(days_to_next_birthday)
             - date::years(person->get_age() + 1);
  auto simulation_time_birthday = Model::get_scheduler()->get_days_to_ymd(ymd);

  // spdlog::info(" age: {}, simulation_time_birthday: {}, days_to_next_birthday: {}", p->get_age(),
  //              simulation_time_birthday, days_to_next_birthday);
  person->set_birthday(simulation_time_birthday);

  if (simulation_time_birthday > 0) {
    spdlog::error(
        "simulation_time_birthday have to be <= 0 when initializing population "
        "(age: {}, days_to_next_birthday: {}, simulation_time_birthday: {})",
        person->get_age(), days_to_next_birthday, simulation_time_birthday);
    throw std::runtime_error("simulation_time_birthday must be <= 0 when initializing population");
  }

  person->schedule_birthday_event(days_to_next_birthday);

  // Use maternal immunity until six months of age.
  if (simulation_time_birthday + (Constants::DAYS_IN_YEAR / 2) >= 0) {
    if (person->get_age() > 0) { spdlog::error("Error in calculating simulation_time_birthday"); }
    person->get_immune_system()->initialize_as_infant();
    // schedule for switch
    person->schedule_switch_immune_system_mode_event(simulation_time_birthday
                                                     + (Constants::DAYS_IN_YEAR / 2));
  }

  const auto &immune_system_parameters = Model::get_config()->get_immune_system_parameters();
  auto immune_value = random->random_beta(immune_system_parameters.get_alpha_immune(),
                                          immune_system_parameters.get_beta_immune());
  person->get_immune_system()->set_latest_immune_value(immune_value);
  person->get_immune_system()->set_increase(false);

  person->set_innate_relative_biting_rate(Person::draw_random_relative_biting_rate(random, config));
  person->update_relative_biting_rate();

  // Cache the moving level to avoid repeated lookups
  auto &movement_settings = config->get_movement_settings();
  person->set_moving_level(
      movement_settings.get_moving_level_generator().draw_random_level(random));

  person->set_latest_update_time(0);

  person->generate_prob_present_at_mda_by_age();

  // spdlog::info("Population::initialize: person {} age {} location {} moving level {}",
  //   i, p->get_age(), loc, p->get_moving_level());

  // Get current values once to avoid repeated calls
  const auto current_relative_biting_rate = person->get_current_relative_biting_rate();
  const auto moving_level = person->get_moving_level();
  const auto &moving_level_value = movement_settings.get_v_moving_level_value()[moving_level];

  individual_relative_biting_by_location_[location].push_back(current_relative_biting_rate);
  individual_relative_moving_by_location_[location].push_back(moving_level_value);

  sum_relative_biting_by_location_[location] += current_relative_biting_rate;
  sum_relative_moving_by_location_[location] += moving_level_value;

  all_alive_persons_by_location_[location].push_back(person.get());
  add_person(std::move(person));
}

void Population::introduce_initial_cases() {
  if (Model::get_instance() != nullptr) {
    for (const auto p_info :
         Model::get_config()->get_genotype_parameters().get_initial_parasite_info()) {
      auto num_of_infections = Model::get_random()->random_poisson(
          std::round(static_cast<double>(size_at(p_info.location)) * p_info.prevalence));
      num_of_infections = num_of_infections <= 0 ? 1 : num_of_infections;

      auto* genotype = Model::get_genotype_db()->at(p_info.parasite_type_id);
      // spdlog::debug("Introducing genotype {} {} with prevalence: {} : {} infections at location
      // {}",
      //           p_info.parasite_type_id, genotype->get_aa_sequence(), p_info.prevalence,
      //           num_of_infections, p_info.location);
      introduce_parasite(p_info.location, genotype, num_of_infections);
    }
    // Recalculate FOI to account for newly introduced infections.
    update_current_foi();

    // update force of infection for N days
    for (auto day = 0; day < Model::get_config()->number_of_tracking_days(); day++) {
      for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
        force_of_infection_for_n_days_by_location_[day][loc] =
            current_force_of_infection_by_location_[loc];
      }
      Model::get_mosquito()->infect_new_cohort_in_prmc(Model::get_config(), Model::get_random(),
                                                       this, day);
    }
  }
}

void Population::introduce_parasite(const int &location,
                                    Genotype* parasite_type,
                                    const int &num_of_infections) {
  if (all_alive_persons_by_location_[location].empty()) {
    // spdlog::debug("introduce_parasite all_alive_persons_by_location location {} is empty",
    // location);
    return;
  }
  auto persons_bitten_today = Model::get_random()->roulette_sampling<Person>(
      num_of_infections, individual_relative_biting_by_location_[location],
      all_alive_persons_by_location_[location], false);

  for (auto* person : persons_bitten_today) { setup_initial_infection(person, parasite_type); }
}

void Population::setup_initial_infection(Person* person, Genotype* parasite_type) {
  person->get_immune_system()->set_increase(true);
  person->set_host_state(Person::ASYMPTOMATIC);

  auto* blood_parasite = person->add_new_parasite_to_blood(parasite_type);

  const auto size = Model::get_random()->random_flat(Model::get_config()
                                                         ->get_parasite_parameters()
                                                         .get_parasite_density_levels()
                                                         .get_log_parasite_density_from_liver(),
                                                     Model::get_config()
                                                         ->get_parasite_parameters()
                                                         .get_parasite_density_levels()
                                                         .get_log_parasite_density_clinical());

  blood_parasite->set_gametocyte_level(
      Model::get_config()->get_epidemiological_parameters().get_gametocyte_level_full());
  blood_parasite->set_last_update_log10_parasite_density(size);

  const auto p_clinical = person->get_probability_progress_to_clinical();
  const auto prob = Model::get_random()->random_flat(0.0, 1.0);

  if (prob < p_clinical) {
    // progress to clinical after several days
    blood_parasite->set_update_function(Model::progress_to_clinical_update_function());
    person->schedule_progress_to_clinical_event(blood_parasite);
  } else {
    // only progress to clearance by Immune system
    // progress to clearance
    blood_parasite->set_update_function(Model::immunity_clearance_update_function());
  }
}

void Population::perform_birth_event_at_location(const int location) {
  const auto poisson_mean = Model::get_config()->get_population_demographic().get_birth_rate()
                            * static_cast<double>(size(location))
                            / static_cast<double>(Constants::DAYS_IN_YEAR);
  const auto number_of_births = Model::get_random()->random_poisson(poisson_mean);
  for (auto i = 0; i < number_of_births; ++i) {
    auto* newborn = give_1_birth(location);
    append_daily_sampling_state(location, newborn);
    Model::get_mdc()->update_person_days_by_years(
        location, Constants::DAYS_IN_YEAR - Model::get_scheduler()->get_current_day_in_year());
  }
}

Person* Population::give_1_birth(const int &location) {
  auto person = std::make_unique<Person>();
  auto* newborn = person.get();
  person->initialize();
  person->set_age(0);
  person->set_host_state(Person::SUSCEPTIBLE);
  person->set_age_class(0);
  person->set_location(location);
  person->set_residence_location(location);
  person->get_immune_system()->initialize_as_infant();
  person->get_immune_system()->set_latest_immune_value(1.0);
  person->get_immune_system()->set_increase(false);

  person->set_latest_update_time(Model::get_scheduler()->current_time());
  //                    p->draw_random_immune();

  // set_relative_biting_rate
  person->set_innate_relative_biting_rate(
      Person::draw_random_relative_biting_rate(Model::get_random(), Model::get_config()));
  person->update_relative_biting_rate();

  person->set_moving_level(
      Model::get_config()->get_movement_settings().get_moving_level_generator().draw_random_level(
          Model::get_random()));

  person->set_birthday(Model::get_scheduler()->current_time());
  const auto number_of_days_to_next_birthday =
      Model::get_scheduler()->get_days_until_next_year_anniversary();
  person->schedule_birthday_event(number_of_days_to_next_birthday);

  // schedule for switch
  person->schedule_switch_immune_system_mode_event(Constants::DAYS_IN_YEAR / 2);

  person->generate_prob_present_at_mda_by_age();

  add_person(std::move(person));
  return newborn;
}

void Population::perform_death_event_at_location(const int location) {
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  if (pi == nullptr) { return; }

  const auto &death_rates =
      Model::get_config()->get_population_demographic().get_death_rate_by_age_class();
  assert(death_rates.size() == Model::get_config()->number_of_age_classes());
  for (auto hs = 0; hs < Person::DEAD; ++hs) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ++ac) {
      const PersonPtrVector candidates = pi->vPerson()[location][hs][ac];
      const auto candidate_count = static_cast<int>(candidates.size());
      if (candidate_count == 0) { continue; }
      const auto poisson_mean =
          candidate_count * death_rates[ac] / static_cast<double>(Constants::DAYS_IN_YEAR);
      const auto number_of_deaths = Model::get_random()->random_poisson(poisson_mean);
      for (auto i = 0; i < number_of_deaths; ++i) {
        const auto index =
            static_cast<std::size_t>(Model::get_random()->random_uniform(candidate_count));
        auto* person = candidates[index];
        if (person->get_host_state() == Person::DEAD) { continue; }
        person->cancel_all_events_except(nullptr);
        person->set_host_state(Person::DEAD);
      }
    }
  }
}

void Population::clear_dead_people_at_location(const int location) {
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  PersonPtrVector dead_people;
  for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ++ac) {
    const auto &dead_by_age = pi->vPerson()[location][Person::DEAD][ac];
    dead_people.insert(dead_people.end(), dead_by_age.begin(), dead_by_age.end());
  }
  for (auto* person : dead_people) {
    remove_from_daily_sampling_state(location, person);
    remove_dead_person(person);
  }
}

IntVector Population::prepare_circulation_context() {
  return Model::get_mdc()->popsize_residence_by_location();
}

void Population::perform_circulation_from_location(const int from_location,
                                                   const IntVector &circulation_context) {
  auto* config = Model::get_config();
  auto &movement_settings = config->get_movement_settings();
  auto &spatial_settings = config->get_spatial_settings();
  auto* random = Model::get_random();
  PersonPtrVector today_circulations;
  auto poisson_means = static_cast<double>(size(from_location))
                       * movement_settings.get_circulation_info().get_circulation_percent();
  if (poisson_means == 0) { return; }
  const auto number_of_circulating_from_this_location = random->random_poisson(poisson_means);
  if (number_of_circulating_from_this_location == 0) { return; }

  // Location-based providers expose their existing dense row. Raster LUT
  // providers return no dense row because movement models use their prepared
  // compact kernels instead.
  static const DoubleVector EMPTY_RELATIVE_DISTANCE_VECTOR;
  const auto* distance_provider =
      Model::get_config()->get_spatial_settings().get_distance_provider();
  const auto* dense_row =
      distance_provider == nullptr ? nullptr : distance_provider->dense_row(from_location);
  const DoubleVector &relative_distance_vector =
      dense_row == nullptr ? EMPTY_RELATIVE_DISTANCE_VECTOR : *dense_row;

  auto v_relative_outmovement_to_destination =
      Model::get_config()
          ->get_movement_settings()
          .get_spatial_model()
          ->get_v_relative_out_movement_to_destination(
              from_location, static_cast<int>(Model::get_config()->number_of_locations()),
              relative_distance_vector, circulation_context);

  std::vector<unsigned int> v_num_leavers_to_destination(
      static_cast<uint64_t>(config->number_of_locations()));

  random->random_multinomial(static_cast<int>(v_relative_outmovement_to_destination.size()),
                             static_cast<unsigned int>(number_of_circulating_from_this_location),
                             v_relative_outmovement_to_destination, v_num_leavers_to_destination);

  for (int target_location = 0; target_location < config->number_of_locations();
       target_location++) {
    // spdlog::info("target_location individual_relative_moving_by_location[{}] size: {} sum:
    // {}",target_location,
    //              individual_relative_moving_by_location[target_location].size(),
    //              sum_relative_moving_by_location[target_location]);
    // std::cout << "num_leave: " << v_num_leavers_to_destination[target_location] << std::endl;
    if (v_num_leavers_to_destination[target_location] == 0) continue;
    // std::cout << " time: " << Model::get_scheduler()->current_time() << "\t from " <<
    // from_location << "\t to " << target_location <<
    // "\t" << v_num_leavers_to_destination[target_location] << std::endl;
    perform_circulation_for_1_location(
        from_location, target_location,
        static_cast<int>(v_num_leavers_to_destination[target_location]), today_circulations);
  }

  for (auto* person : today_circulations) { person->randomly_choose_target_location(); }
}

void Population::perform_circulation_for_1_location(const int &from_location,
                                                    const int &target_location,
                                                    const int &number_of_circulations,
                                                    std::vector<Person*> &today_circulations) {
  // spdlog::info("perform_circulation_for_1_location individual_relative_moving_by_location[{}]
  // size: {} sum: {}",target_location,
  //              individual_relative_moving_by_location[target_location].size(),
  //              sum_relative_moving_by_location[target_location]);

  auto persons_moving_today = Model::get_random()->roulette_sampling<Person>(
      number_of_circulations, individual_relative_moving_by_location_[from_location],
      all_alive_persons_by_location_[from_location], false,
      sum_relative_moving_by_location_[from_location]);

  for (auto* person : persons_moving_today) {
    assert(person->get_host_state() != Person::DEAD);

    // if that person age is less than 18 then do another random to decide
    // whether they move or not
    if (person->get_age() <= 18) {
      auto prob = Model::get_config()
                      ->get_movement_settings()
                      .get_circulation_info()
                      .get_relative_probability_that_child_travels_compared_to_adult();
      if (prob < 1.0 && Model::get_random()->random_flat(0, 1) > prob) {
        // that child does not move
        continue;
      }
    }

    person->get_today_target_locations().push_back(target_location);
    today_circulations.push_back(person);
  }
}

bool Population::has_0_case() {
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  auto* config = Model::get_config();
  const auto number_of_locations = config->number_of_locations();
  const auto number_of_age_classes = config->number_of_age_classes();
  auto &persons_by_location_state_age = pi->vPerson();
  for (int loc = 0; loc < number_of_locations; loc++) {
    for (int hs = Person::EXPOSED; hs <= Person::CLINICAL; hs++) {
      for (int ac = 0; ac < number_of_age_classes; ac++) {
        if (!persons_by_location_state_age[loc][hs][ac].empty()) { return false; }
      }
    }
  }
  return true;
}

void Population::execute_all_individual_events(int up_to_time) {
  if (all_persons_ == nullptr) {
    throw std::runtime_error(
        "PersonIndexAll not found in Population::update_all_individual_events");
  }
  for (auto &person_ptr : all_persons_->v_person()) {
    if (person_ptr) {  // Optional check for the unique_ptr itself
      if (person_ptr->get_host_state() == Person::DEAD) { continue; }
      person_ptr->update_events(up_to_time);
    }
  }
}

void Population::persist_force_of_infection_at_location(const int location,
                                                        const int tracking_index) {
  force_of_infection_for_n_days_by_location_[tracking_index][location] =
      current_force_of_infection_by_location_[location];
}

void Population::reset_daily_sampling_state(const int location) {
  individual_foi_by_location_[location].clear();
  individual_relative_biting_by_location_[location].clear();
  individual_relative_moving_by_location_[location].clear();
  all_alive_persons_by_location_[location].clear();
  current_force_of_infection_by_location_[location] = 0.0;
  sum_relative_biting_by_location_[location] = 0.0;
  sum_relative_moving_by_location_[location] = 0.0;
}

void Population::append_daily_sampling_state(const int location, Person* person) {
  const auto log_10_total_infectious_density =
      person->get_all_clonal_parasite_populations()->log10_total_infectious_density();
  const auto relative_biting = person->get_current_relative_biting_rate();
  const auto person_foi =
      log_10_total_infectious_density == ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY
          ? 0.0
          : relative_biting * Person::relative_infectivity(log_10_total_infectious_density);
  const auto relative_moving = Model::get_config()
                                   ->get_movement_settings()
                                   .get_v_moving_level_value()[person->get_moving_level()];

  individual_foi_by_location_[location].push_back(person_foi);
  individual_relative_biting_by_location_[location].push_back(relative_biting);
  individual_relative_moving_by_location_[location].push_back(relative_moving);
  all_alive_persons_by_location_[location].push_back(person);
  current_force_of_infection_by_location_[location] += person_foi;
  sum_relative_biting_by_location_[location] += relative_biting;
  sum_relative_moving_by_location_[location] += relative_moving;
}

void Population::remove_from_daily_sampling_state(const int location, Person* person) {
  auto &alive_people = all_alive_persons_by_location_[location];
  const auto person_it = std::ranges::find(alive_people, person);
  if (person_it == alive_people.end()) { return; }
  // Iterator distances use the container's signed difference_type.
  const auto offset = std::distance(alive_people.begin(), person_it);

  // Safe because person_it is in [begin(), end()).
  const auto index = static_cast<std::size_t>(offset);

  current_force_of_infection_by_location_[location] -= individual_foi_by_location_[location][index];
  sum_relative_biting_by_location_[location] -=
      individual_relative_biting_by_location_[location][index];
  sum_relative_moving_by_location_[location] -=
      individual_relative_moving_by_location_[location][index];
  individual_foi_by_location_[location].erase(individual_foi_by_location_[location].begin()
                                              + offset);
  individual_relative_biting_by_location_[location].erase(
      individual_relative_biting_by_location_[location].begin() + offset);
  individual_relative_moving_by_location_[location].erase(
      individual_relative_moving_by_location_[location].begin() + offset);
  alive_people.erase(person_it);
}

void Population::update_people_and_append_sampling_state(const int location) {
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  PersonPtrVector people;
  people.reserve(size(location));
  for (auto hs = 0; hs < Person::DEAD; ++hs) {
    for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ++ac) {
      const auto &people_by_age = pi->vPerson()[location][hs][ac];
      people.insert(people.end(), people_by_age.begin(), people_by_age.end());
    }
  }
  for (auto* person : people) {
    person->update();
    append_daily_sampling_state(location, person);
  }
}

void Population::prepare_daily_state_at_location(const int location) {
  reset_daily_sampling_state(location);
  update_people_and_append_sampling_state(location);
  perform_death_event_at_location(location);
  clear_dead_people_at_location(location);
  perform_birth_event_at_location(location);
}

void Population::update_current_foi() {
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  const auto number_of_locations = Model::get_config()->number_of_locations();
  const auto number_of_age_classes = Model::get_config()->number_of_age_classes();
  auto &persons_by_location_state_age = pi->vPerson();
  for (int location = 0; location < number_of_locations; location++) {
    reset_daily_sampling_state(location);
    for (int hs = 0; hs < Person::DEAD; hs++) {
      for (int ac = 0; ac < number_of_age_classes; ac++) {
        for (auto* person : persons_by_location_state_age[location][hs][ac]) {
          append_daily_sampling_state(location, person);
        }
      }
    }
  }
}
