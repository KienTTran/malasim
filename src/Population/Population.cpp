#include "Population.h"

#include <Simulation/Model.h>
#include <Utils/Random.h>
#include <spdlog/spdlog.h>

#include <cfloat>
#include <memory>

#include "ClinicalUpdateFunction.h"
#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Events/BirthdayEvent.h"
#include "Events/SwitchImmuneComponentEvent.h"
#include "ImmuneSystem/ImmuneSystem.h"
#include "ImmuneSystem/InfantImmuneComponent.h"
#include "ImmuneSystem/NonInfantImmuneComponent.h"
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
      // spdlog::info("individual_relative_moving_by_location[{}] size: {} sum: {}",loc,
      //              individual_relative_moving_by_location[loc].size(),
      //              sum_relative_moving_by_location[loc]);
    }
  }
}

void Population::initialize_person_indices() {
  const int number_of_location = Model::get_config()->number_of_locations();
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
  popsize_by_location_[person->get_location()]++;
  // all_persons will take ownership
  all_persons_->add(std::move(person));
}

void Population::remove_dead_person(Person* person) { remove_person(person); }

void Population::remove_person(Person* person) {
  // persons_.erase(std::ranges::remove(persons_, person).begin(), persons_.end());
  popsize_by_location_[person->get_location()]--;
  for (auto &person_index : *person_index_list_) { person_index->remove(person); }
  all_persons_->remove(person);
}

void Population::notify_change(Person* person, const Person::Property &property,
                               const void* old_value, const void* new_value) {
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
  if (age_class == -1) {
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

std::size_t Population::size(const int &location, const Person::HostStates &hs,
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

void Population::perform_infection_event() {
  PersonPtrVector today_infections;

  const auto tracking_index =
      Model::get_scheduler()->current_time() % Model::get_config()->number_of_tracking_days();

  for (int loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    const double foi = force_of_infection_for_n_days_by_location_[tracking_index][loc];
    if (foi <= DBL_EPSILON) continue;

    const double new_beta =
        Model::get_config()->location_db()[loc].beta *
        Model::get_config()->get_seasonality_settings().get_seasonal_factor(
            Model::get_scheduler()->get_calendar_date(), loc);

    const double poisson_means = new_beta * foi;
    const int number_of_bites = Model::get_random()->random_poisson(poisson_means);
    if (number_of_bites <= 0) continue;

    // Stats
    Model::get_mdc()->collect_number_of_bites(loc, number_of_bites);

    // Sampling guards
    if (all_alive_persons_by_location_[loc].empty()) {
      spdlog::trace("all_alive_persons_by_location location {} is empty", loc);
      continue;
    }
    if (sum_relative_biting_by_location_[loc] <= 0.0) {
      spdlog::trace("sum_relative_biting_by_location[{}] is zero", loc);
      continue;
    }

    // Draw bite recipients
    auto persons_bitten_today = Model::get_random()->roulette_sampling<Person>(
        number_of_bites,
        individual_relative_biting_by_location_[loc],
        all_alive_persons_by_location_[loc],
        /*allow_repetition=*/false,
        sum_relative_biting_by_location_[loc]);

    // Early guard on mosquito table
    if (Model::get_mosquito()->genotypes_table[tracking_index][loc].empty()) {
      spdlog::trace("mosquito genotypes_table[{}][{}] is empty", tracking_index, loc);
      continue;
    }

    const bool use_challenge =
        Model::get_config()->get_transmission_settings().get_transmission_parameter() > 0.0;

    for (auto* person : persons_bitten_today) {
      assert(person->get_host_state() != Person::DEAD);
      person->increase_number_of_times_bitten();

      const int genotype_id = Model::get_mosquito()->random_genotype(loc, tracking_index);
      if (genotype_id < 0) continue; // extra safety

      // Draw once per bite
      const double draw = Model::get_random()->random_flat(0.0, 1.0);

      bool infected = false;
      if (use_challenge) {
        // v4-style immunity curve, but safe/clamped
        double theta = person->get_immune_system()->get_current_value();
        theta = std::clamp(theta, 0.0, 1.0);

        double pr = Model::get_config()->get_transmission_settings().get_transmission_parameter();
        double t = (theta - 0.2) / 0.6;
        t = std::clamp(t, 0.0, 1.0); // only blend on [0.2,0.8] region

        double pr_inf = pr * (1.0 - t) + 0.1 * t;
        if (theta > 0.8) pr_inf = 0.1;
        if (theta < 0.2) pr_inf = pr;
        pr_inf = std::clamp(pr_inf, 0.0, 1.0);

        infected = (draw <= pr_inf);
      } else {
        if (Model::get_config()
                ->get_epidemiological_parameters()
                .get_using_variable_probability_infectious_bites_cause_infection()) {
          infected = (draw <= person->p_infection_from_an_infectious_bite());
        } else {
          infected = (draw <= Model::get_config()
                               ->get_transmission_settings()
                               .get_p_infection_from_an_infectious_bite());
        }
      }

      if (infected &&
          person->get_host_state() != Person::EXPOSED &&
          person->liver_parasite_type() == nullptr) {
        person->get_today_infections().push_back(genotype_id);
        today_infections.push_back(person);
      }
    }
  }

  if (today_infections.empty()) return;

  for (auto* person : today_infections) {
    if (!person->get_today_infections().empty()) {
      Model::get_mdc()->record_1_infection(person->get_location());
    }
    person->randomly_choose_parasite();
  }

  today_infections.clear();
}


void Population::generate_individual(int location, int age_class) {
  auto person = std::make_unique<Person>();
  person->initialize();

  person->set_location(location);
  person->set_residence_location(location);
  person->set_host_state(Person::SUSCEPTIBLE);

  // Set the age of the individual, which also sets the age class. Note that we
  // are defining the types to conform to the signature of random_uniform<int>
  uint age_from = (age_class == 0) ? 0
                                   : Model::get_config()
                                         ->get_population_demographic()
                                         .get_initial_age_structure()[age_class - 1];
  uint age_to =
      Model::get_config()->get_population_demographic().get_initial_age_structure()[age_class];
  person->set_age(static_cast<int>(Model::get_random()->random_uniform<int>(
      static_cast<int>(age_from), static_cast<int>(age_to) + 1)));

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
    spdlog::error("simulation_time_birthday have to be <= 0 when initializing population");
  }

  person->schedule_birthday_event(days_to_next_birthday);

  // set immune component at 6 months
  if (simulation_time_birthday + Constants::DAYS_IN_YEAR / 2 >= 0) {
    if (person->get_age() > 0) { spdlog::error("Error in calculating simulation_time_birthday"); }
    person->get_immune_system()->set_immune_component(std::make_unique<InfantImmuneComponent>());
    // schedule for switch
    person->schedule_switch_immune_component_event(simulation_time_birthday
                                                   + (Constants::DAYS_IN_YEAR / 2));
  } else {
    // LOG(INFO) << "Adult: " << p->age() << " - " << simulation_time_birthday;
    person->get_immune_system()->set_immune_component(std::make_unique<NonInfantImmuneComponent>());
  }

  auto immune_value = Model::get_random()->random_beta(
      Model::get_config()->get_immune_system_parameters().alpha_immune,
      Model::get_config()->get_immune_system_parameters().beta_immune);
  person->get_immune_system()->immune_component()->set_latest_value(immune_value);
  person->get_immune_system()->set_increase(false);

  person->set_innate_relative_biting_rate(
      Person::draw_random_relative_biting_rate(Model::get_random(), Model::get_config()));
  person->update_relative_biting_rate();

  // Cache the moving level to avoid repeated lookups
  auto &movement_settings = Model::get_config()->get_movement_settings();
  person->set_moving_level(
      movement_settings.get_moving_level_generator().draw_random_level(Model::get_random()));

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
    // Reset and calculate FOI based on the current population state (pre-infection introduction).
    update_current_foi();
    // std::cout << Model::CONFIG->initial_parasite_info().size() << std::endl;
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
      Model::get_mosquito()->infect_new_cohort_in_PRMC(Model::get_config(), Model::get_random(),
                                                       this, day);
    }
  }
}

void Population::introduce_parasite(const int &location, Genotype* parasite_type,
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

void Population::perform_birth_event() {
  //    std::cout << "Birth Event" << std::endl;
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    auto poisson_means = Model::get_config()->get_population_demographic().get_birth_rate()
                         * static_cast<double>(size(loc))
                         / static_cast<double>(Constants::DAYS_IN_YEAR);
    const auto number_of_births = Model::get_random()->random_poisson(poisson_means);
    for (auto i = 0; i < number_of_births; i++) {
      give_1_birth(loc);
      Model::get_mdc()->update_person_days_by_years(
          loc, Constants::DAYS_IN_YEAR - Model::get_scheduler()->get_current_day_in_year());
    }
  }
  //    std::cout << "End Birth Event" << std::endl;
}

void Population::give_1_birth(const int &location) {
  auto person = std::make_unique<Person>();
  person->initialize();
  person->set_age(0);
  person->set_host_state(Person::SUSCEPTIBLE);
  person->set_age_class(0);
  person->set_location(location);
  person->set_residence_location(location);
  person->get_immune_system()->set_immune_component(std::make_unique<InfantImmuneComponent>());
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
  const auto number_of_days_to_next_birthday = Model::get_scheduler()->get_days_to_next_year();
  person->schedule_birthday_event(number_of_days_to_next_birthday);

  // schedule for switch
  person->schedule_switch_immune_component_event(Constants::DAYS_IN_YEAR / 2);

  person->generate_prob_present_at_mda_by_age();

  add_person(std::move(person));
}

void Population::perform_death_event() {
  //    std::cout << "Death Event" << std::endl;
  // simply change state to dead and release later
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  if (pi == nullptr) return;

  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      if (hs == Person::DEAD) continue;
      for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        const int size = static_cast<int>(pi->vPerson()[loc][hs][ac].size());
        if (size == 0) continue;
        auto poisson_means =
            size
            * Model::get_config()->get_population_demographic().get_death_rate_by_age_class()[ac]
            / static_cast<double>(Constants::DAYS_IN_YEAR);

        assert(
            Model::get_config()->get_population_demographic().get_death_rate_by_age_class().size()
            == Model::get_config()->number_of_age_classes());
        const auto number_of_deaths = Model::get_random()->random_poisson(poisson_means);
        if (number_of_deaths == 0) continue;

        //                std::cout << numberOfDeaths << std::endl;
        for (int i = 0; i < number_of_deaths; i++) {
          // change state to Death;
          const int index = static_cast<int>(Model::get_random()->random_uniform(size));
          //                    std::cout << index << "-" << pi->vPerson()[loc][hs][ac].size() <<
          //                    std::endl;
          auto* person = pi->vPerson()[loc][hs][ac][index];
          person->cancel_all_events_except(nullptr);
          person->set_host_state(Person::DEAD);
        }
      }
    }
  }
  clear_all_dead_state_individual();
}

void Population::clear_all_dead_state_individual() {
  // return all Death to object pool and clear vPersonIndex[l][dead][ac] for all location and ac
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  PersonPtrVector remove_persons;

  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      for (auto* person : pi->vPerson()[loc][Person::DEAD][ac]) {
        remove_persons.push_back(person);
      }
    }
  }

  for (Person* person : remove_persons) { remove_dead_person(person); }
}

void Population::perform_circulation_event() {
  // for each location
  //  get number of circulations based on size * circulation_percent
  //  distributes that number into others location based of other location size
  //  for each number in that list select an individual, and schedule a movement event on next day
  // if (Model::get_config()->number_of_locations() == 1) { return; }
  PersonPtrVector today_circulations;

  std::vector<int> v_number_of_residents_by_location(Model::get_config()->number_of_locations(), 0);

  for (auto location = 0; location < Model::get_config()->number_of_locations(); location++) {
    //        v_number_of_residents_by_location[target_location] = (size(target_location));
    v_number_of_residents_by_location[location] =
        Model::get_mdc()->popsize_residence_by_location()[location];
    //        std::cout << v_original_pop_size_by_location[target_location] << std::endl;
  }

  for (int from_location = 0; from_location < Model::get_config()->number_of_locations();
       from_location++) {
    auto poisson_means = static_cast<double>(size(from_location))
                         * Model::get_config()
                               ->get_movement_settings()
                               .get_circulation_info()
                               .get_circulation_percent();
    if (poisson_means == 0) continue;
    const auto number_of_circulating_from_this_location =
        Model::get_random()->random_poisson(poisson_means);
    if (number_of_circulating_from_this_location == 0) continue;

    DoubleVector v_relative_outmovement_to_destination(Model::get_config()->number_of_locations(),
                                                       0);
    v_relative_outmovement_to_destination =
        Model::get_config()
            ->get_movement_settings()
            .get_spatial_model()
            ->get_v_relative_out_movement_to_destination(
                from_location, Model::get_config()->number_of_locations(),
                Model::get_config()
                    ->get_spatial_settings()
                    .get_spatial_distance_matrix()[from_location],
                v_number_of_residents_by_location);

    std::vector<unsigned int> v_num_leavers_to_destination(
        static_cast<uint64_t>(Model::get_config()->number_of_locations()));

    Model::get_random()->random_multinomial(
        static_cast<int>(v_relative_outmovement_to_destination.size()),
        static_cast<unsigned int>(number_of_circulating_from_this_location),
        v_relative_outmovement_to_destination, v_num_leavers_to_destination);

    for (int target_location = 0; target_location < Model::get_config()->number_of_locations();
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
  }

  for (auto* person : today_circulations) { person->randomly_choose_target_location(); }

  today_circulations.clear();
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
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int hs = Person::EXPOSED; hs <= Person::CLINICAL; hs++) {
      for (int ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        if (!pi->vPerson()[loc][hs][ac].empty()) { return false; }
      }
    }
  }
  return true;
}

void Population::update_all_individuals() {
  // update all individuals
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (int hs = 0; hs < Person::DEAD; hs++) {
      for (int ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        for (auto* person : pi->vPerson()[loc][hs][ac]) { person->update(); }
      }
    }
  }
  // if (all_persons_ == nullptr) {
  //   throw std::runtime_error("PersonIndexAll not found in Population::update_all_individuals");
  // }
  // for (auto &person_ptr : all_persons_->v_person()) {
  //   if (person_ptr) {
  //     if (person_ptr->get_host_state() == Person::DEAD) { continue; }
  //     person_ptr->update();
  //   }
  // }
}

// TODO: it should be called "execute_all_individual_events" for an input time
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

void Population::persist_current_force_of_infection_to_use_n_days_later() {
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    force_of_infection_for_n_days_by_location_[Model::get_scheduler()->current_time()
                                               % Model::get_config()->number_of_tracking_days()]
                                              [loc] = current_force_of_infection_by_location_[loc];
  }
}

void Population::update_current_foi() {
  auto* pi = get_person_index<PersonIndexByLocationStateAgeClass>();
  for (int location = 0; location < Model::get_config()->number_of_locations(); location++) {
    // spdlog::info("location {} pop {}", location, size(location));
    // reset force of infection for each location
    current_force_of_infection_by_location_[location] = 0.0;
    sum_relative_biting_by_location_[location] = 0.0;
    sum_relative_moving_by_location_[location] = 0.0;

    // using clear so as system will not reallocate memory slot for vector
    individual_foi_by_location_[location].clear();
    individual_relative_biting_by_location_[location].clear();
    individual_relative_moving_by_location_[location].clear();
    all_alive_persons_by_location_[location].clear();

    for (int hs = 0; hs < Person::DEAD; hs++) {
      for (int ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        // spdlog::info("There are {} individuals in location {} with host state {} and age class
        // {}", pi->vPerson()[location][hs][ac].size(), location, hs, ac); for (std::size_t i = 0; i
        // < pi->vPerson()[location][hs][ac].size(); i++) { Person* person =
        // pi->vPerson()[location][hs][ac][i];
        for (auto* person : (pi->vPerson()[location][hs][ac])) {
          double log_10_total_infectious_density =
              person->get_all_clonal_parasite_populations()->log10_total_infectious_density();
          const auto person_relative_biting_rate = person->get_current_relative_biting_rate();
          auto individual_foi =
              log_10_total_infectious_density == ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY
                  ? 0.0
                  : person_relative_biting_rate
                        * Person::relative_infectivity(log_10_total_infectious_density);

          individual_foi_by_location_[location].push_back(individual_foi);
          individual_relative_biting_by_location_[location].push_back(person_relative_biting_rate);
          const auto &moving_level_value =
              Model::get_config()
                  ->get_movement_settings()
                  .get_v_moving_level_value()[person->get_moving_level()];
          individual_relative_moving_by_location_[location].push_back(moving_level_value);

          sum_relative_biting_by_location_[location] += person_relative_biting_rate;
          sum_relative_moving_by_location_[location] += moving_level_value;
          current_force_of_infection_by_location_[location] += individual_foi;
          all_alive_persons_by_location_[location].push_back(person);
          // spdlog::info("person {} individual_relative_biting_by_location[{}]
          // {}",person->get_id(), location,
          //              individual_relative_biting_by_location[location].back());
        }
      }
    }
    // spdlog::info("update_current_foi individual_relative_moving_by_location[{}] size: {} sum:
    // {}",location,
    //                individual_relative_moving_by_location[location].size(),
    //                sum_relative_moving_by_location[location]);
    // spdlog::info("update_current_foi all_alive_persons_by_location[{}] size: {}",location,
    //                all_alive_persons_by_location[location].size());
  }
}
