#include "Person.h"

#include <Configuration/Config.h>
#include <Simulation/Model.h>
#include <Utils/Helpers/TimeHelpers.h>
#include <Utils/Random.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>

#include "Core/Scheduler/Scheduler.h"
#include "Events/BirthdayEvent.h"
#include "Events/CirculateToTargetLocationNextDayEvent.h"
#include "Events/EndClinicalByNoTreatmentEvent.h"
#include "Events/EndClinicalEvent.h"
#include "Events/MatureGametocyteEvent.h"
#include "Events/MoveParasiteToBloodEvent.h"
#include "Events/ProgressToClinicalEvent.h"
#include "Events/RaptEvent.h"
#include "Events/ReceiveMDATherapyEvent.h"
#include "Events/ReceiveTherapyEvent.h"
#include "Events/ReportTreatmentFailureDeathEvent.h"
#include "Events/ReturnToResidenceEvent.h"
#include "Events/SwitchImmuneComponentEvent.h"
#include "Events/TestTreatmentFailureEvent.h"
#include "Events/UpdateWhenDrugIsPresentEvent.h"
#include "MDC/ModelDataCollector.h"
#include "Population/ClinicalUpdateFunction.h"
#include "Population/DrugsInBlood.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Population.h"
#include "Treatment/Therapies/Drug.h"
#include "Treatment/Therapies/MACTherapy.h"
#include "Utils/Constants.h"

Person::Person() {
  immune_system_ = std::make_unique<ImmuneSystem>(this);
  drugs_in_blood_ = std::make_unique<DrugsInBlood>(this);
  all_clonal_parasite_populations_ = std::make_unique<SingleHostClonalParasitePopulations>(this);
};

Person::~Person() = default;

void Person::initialize() {
  event_manager_.initialize();

  all_clonal_parasite_populations_->init();

  drugs_in_blood_->init();

  today_infections_.clear();
  today_target_locations_.clear();
  starting_drug_values_for_mac_.clear();

  innate_relative_biting_rate_ = 0;
  current_relative_biting_rate_ = 0;
}

void Person::notify_change(const Property &property, const void* old_value, const void* new_value) {
  if (population_ != nullptr) { population_->notify_change(this, property, old_value, new_value); }
}

void Person::set_location(const int &value) {
  if (location_ != value) {
    if (Model::get_mdc() != nullptr) {
      const auto day_diff =
          (Constants::DAYS_IN_YEAR - Model::get_scheduler()->get_current_day_in_year());
      if (location_ != -1) { Model::get_mdc()->update_person_days_by_years(location_, -day_diff); }
      Model::get_mdc()->update_person_days_by_years(value, day_diff);
    }

    notify_change(LOCATION, &location_, &value);

    location_ = value;
  }
}

void Person::set_host_state(const HostStates &value) {
  if (host_state_ != value) {
    notify_change(HOST_STATE, &host_state_, &value);
    if (value == DEAD) {
      // clear also remove all infection forces
      all_clonal_parasite_populations_->clear();
      // TODO: remove all events
      Model::get_mdc()->record_1_death(location_, birthday_, number_of_times_bitten_, age_class_,
                                       static_cast<int>(age_));
    }

    host_state_ = value;
  }
}

void Person::set_age(const uint &value) {
  if (age_ != value) {
    // TODO::if age access the limit of age structure i.e. 100, remove person???

    notify_change(AGE, &age_, &value);
    // update biting rate level
    age_ = value;

    // update age class
    if (Model::get_instance() != nullptr) {
      auto ac = age_class_ == -1 ? 0 : age_class_;
      while (ac < (Model::get_config()->number_of_age_classes() - 1)
             && age_ >= Model::get_config()->age_structure()[ac]) {
        ac++;
      }
      set_age_class(ac);
    }
  }
}

void Person::set_age_class(const int &value) {
  if (age_class_ != value) {
    notify_change(AGE_CLASS, &age_class_, &value);
    age_class_ = value;
  }
}

void Person::set_moving_level(int value) {
  if (moving_level_ != value) {
    notify_change(MOVING_LEVEL, &moving_level_, &value);
    moving_level_ = value;
  }
}

void Person::set_immune_system(std::unique_ptr<ImmuneSystem> value) {
  immune_system_ = std::move(value);
}

ClonalParasitePopulation* Person::add_new_parasite_to_blood(Genotype* parasite_type) const {
  auto blood_parasite = std::make_unique<ClonalParasitePopulation>(parasite_type);
  auto* raw_ptr = blood_parasite.get();

  blood_parasite->set_last_update_log10_parasite_density(
      Model::get_config()
          ->get_parasite_parameters()
          .get_parasite_density_levels()
          .get_log_parasite_density_from_liver());

  all_clonal_parasite_populations_->add(std::move(blood_parasite));
  return raw_ptr;
}

double Person::relative_infectivity(const double& log10_parasite_density) {
  if (log10_parasite_density == ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY) return 0.0;

  // this sigma has already taken 'ln' and 'log10' into account
  const auto d_n = (log10_parasite_density * Model::get_config()->get_epidemiological_parameters().get_relative_infectivity().get_sigma())
                   + Model::get_config()->get_epidemiological_parameters().get_relative_infectivity().get_ro_star();
  const auto p = Model::get_random()->cdf_standard_normal_distribution(d_n);

  return (p * p) + 0.01;
}

double Person::get_probability_progress_to_clinical() {
  return immune_system_->get_clinical_progression_probability();
}

void Person::cancel_all_other_progress_to_clinical_events_except(PersonEvent* in_event) {
  cancel_all_events_except<ProgressToClinicalEvent>(in_event);
}

void Person::change_all_parasite_update_function(ParasiteDensityUpdateFunction* from,
                                                 ParasiteDensityUpdateFunction* to) const {
  all_clonal_parasite_populations_->change_all_parasite_update_function(from, to);
}

bool Person::will_progress_to_death_when_receive_no_treatment() {
  // yes == death
  const auto prob = Model::get_random()->random_flat(0.0, 1.0);
  return prob <= Model::get_config()
                     ->get_population_demographic()
                     .get_mortality_when_treatment_fail_by_age_class()[age_class_];
}

bool Person::will_progress_to_death_when_recieve_treatment() {
  // yes == death
  double prob = Model::get_random()->random_flat(0.0, 1.0);
  // 90% lower than no treatment
  return prob <= Model::get_config()
                         ->get_population_demographic()
                         .get_mortality_when_treatment_fail_by_age_class()[age_class_]
                     * 0.1;
}

int Person::complied_dosing_days(const int &dosing_day) {
  if (Model::get_config()->get_epidemiological_parameters().get_p_compliance() < 1) {
    const auto prob = Model::get_random()->random_flat(0.0, 1.0);
    if (prob > Model::get_config()->get_epidemiological_parameters().get_p_compliance()) {
      // do not comply
      const auto weight =
          (Model::get_config()->get_epidemiological_parameters().get_min_dosing_days() - dosing_day)
          / (1 - Model::get_config()->get_epidemiological_parameters().get_p_compliance());
      return static_cast<int>(std::ceil(
          (weight * prob)
          + Model::get_config()->get_epidemiological_parameters().get_min_dosing_days() - weight));
    }
  }
  return dosing_day;
}

int Person::complied_dosing_days(const SCTherapy* therapy) {
  // Return the max day if we have full compliance
  if (therapy->full_compliance()) { return therapy->get_max_dosing_day(); }

  // Roll the dice
  auto rv = Model::get_random()->random_flat(0.0, 1.0);

  // Otherwise, iterate through the probabilities that they will complete the
  // therapy on the given day
  auto upper_bound = therapy->pr_completed_days[0];
  for (auto days = 1; days < therapy->pr_completed_days.size() + 1; days++) {
    if (rv < upper_bound) { return days; }
    upper_bound += therapy->pr_completed_days[days];
  }

  // We encountered an error, this should not happen
  throw std::runtime_error("Bounds of pr_completed_days exceeded: rv = " + std::to_string(rv));
}

/*
 * From Temple Malaria Simulation Person.cpp
 */
// Give the therapy indicated to the individual, making note of the parasite
// that caused the clinical case. Note that we assume that MACTherapy is going
// to be fairly rare, but that additional bookkeeping needs to be done in the
// event of one.
void Person::receive_therapy(Therapy* therapy, ClonalParasitePopulation* clinical_caused_parasite,
                             bool is_part_of_mac_therapy, bool is_public_sector) {
  // Start by checking if this is a simple therapy with a single dosing regime
  auto* sc_therapy = dynamic_cast<SCTherapy*>(therapy);
  if (sc_therapy != nullptr) {
    receive_therapy(sc_therapy, is_part_of_mac_therapy);
  } else {
    // This is not a simple therapy, multiple treatments and dosing regimes may
    // be involved
    auto* mac_therapy = dynamic_cast<MACTherapy*>(therapy);
    assert(mac_therapy != nullptr);

    starting_drug_values_for_mac_.clear();
    for (std::size_t i = 0; i < mac_therapy->get_therapy_ids().size(); i++) {
      const auto therapy_id = mac_therapy->get_therapy_ids()[i];
      const auto start_day = mac_therapy->get_start_at_days()[i];
      assert(start_day >= 1);

      // Verify the therapy that is part of the regimen
      sc_therapy = dynamic_cast<SCTherapy*>(Model::get_therapy_db()[therapy_id].get());
      if (sc_therapy == nullptr) {
        auto message = "Complex therapy (" + std::to_string(therapy->get_id())
                       + ") contains a reference to an unknown therapy id ("
                       + std::to_string(therapy_id) + ")";
        throw std::runtime_error(message);
      }
      if (!sc_therapy->full_compliance()) {
        auto message = "Complex therapy (" + std::to_string(therapy->get_id())
                       + ") contains a reference to a therapy (" + std::to_string(therapy_id)
                       + ") that has variable compliance";
        throw std::runtime_error(message);
      }

      if (start_day == 1) {
        receive_therapy(sc_therapy, true);
      } else {
        schedule_receive_therapy_event(clinical_caused_parasite, sc_therapy, start_day - 1, true);
      }
    }
  }

  last_therapy_id_ = therapy->get_id();
  if (is_public_sector) {
    latest_time_received_public_treatment_ = Model::get_scheduler()->current_time();
  }
}

void Person::receive_therapy(SCTherapy* sc_therapy, bool is_mac_therapy) {
  // Determine the dosing days
  auto dosing_days = complied_dosing_days(sc_therapy);

  // Add the treatment to the blood
  for (int drug_id : sc_therapy->drug_ids) {
    add_drug_to_blood(Model::get_drug_db()->at(drug_id).get(), dosing_days, is_mac_therapy);
  }
}

void Person::add_drug_to_blood(DrugType* dt, const int &dosing_days, bool is_part_of_mac_therapy) {
  // Prepare the drug object
  auto drug = std::make_unique<Drug>(dt);
  drug->set_dosing_days(dosing_days);
  drug->set_last_update_time(Model::get_scheduler()->current_time());

  // Find the mean and standard deviation for the drug, and use those values to
  // determine the drug level for this individual
  const auto sd = dt->age_group_specific_drug_concentration_sd()[age_class_];
  const auto mean_drug_absorption = dt->age_specific_drug_absorption()[age_class_];
  double drug_level = Model::get_random()->random_normal_truncated(mean_drug_absorption, sd);

  // If this is going to be part of a complex therapy regime then we need to
  // note this initial drug level
  if (is_part_of_mac_therapy) {
    if (drugs_in_blood_->contains(dt->id())) {
      // Long half-life drugs are already present in the blood
      drug_level = drugs_in_blood_->at(dt->id())->starting_value();
    } else if (starting_drug_values_for_mac_.contains(dt->id())) {
      // Short half-life drugs that were taken, but cleared the blood already
      drug_level = starting_drug_values_for_mac_[dt->id()];
    }
    // Note the value for future use
    starting_drug_values_for_mac_[dt->id()] = drug_level;
  }

  // Set the starting level for this course of treatment
  drug->set_starting_value(drug_level);

  if (drugs_in_blood_->contains(dt->id())) {
    drug->set_last_update_value(drugs_in_blood_->at(dt->id())->last_update_value());
  } else {
    drug->set_last_update_value(0.0);
  }

  drug->set_start_time(Model::get_scheduler()->current_time());
  drug->set_end_time(Model::get_scheduler()->current_time()
                     + dt->get_total_duration_of_drug_activity(dosing_days));

  drugs_in_blood_->add_drug(std::move(drug));
}

void Person::change_state_when_no_parasite_in_blood() {
  if (all_clonal_parasite_populations_->size() == 0) {
    if (liver_parasite_type_ == nullptr) {
      set_host_state(SUSCEPTIBLE);
    } else {
      set_host_state(EXPOSED);
    }
    immune_system_->set_increase(false);
  }
}

/**
 * Calculate the probability of developing clinical symptoms in recrudescent
 * infections based on malaria prevalence (PfPR2-10) and whether the trial
 * enrolled only young children.
 *
 * Parameters are taken from the paper:
 * Mumtaz, R., Okell, L.C. & Challenger, J.D. Asymptomatic recrudescence after
 * artemether–lumefantrine treatment for uncomplicated falciparum malaria: a
 * systematic review and meta-analysis. Malar J 19, 453 (2020).
 * https://doi.org/10.1186/s12936-020-03520-1
 *
 * @param pfpr Malaria prevalence (PfPR2-10) as a percentage (e.g., 10 for 10%)
 * @param enrollYoungChildren Boolean indicating if the trial enrolled only
 * young children
 * @return Probability of symptomatic recrudescences as a percentage
 */
double calculate_symptomatic_recrudescence_probability(double pfpr,
                                                       bool is_young_children = false) {
  // Base probability for adults at 0% PfPR
  const double base_probability = 52.0;

  // Calculate the odds ratio reduction for increase in PfPR
  const double reduction_factor = 1.17;
  const double odd_ration_factor_for_young_children = 1.61;

  // Calculate the odds for the given PfPR
  double odds_ratio = pow((1 / reduction_factor), (pfpr / 10));

  if (is_young_children) {
    // Adjust odds ratio for young children
    odds_ratio *= odd_ration_factor_for_young_children;
  }

  // Convert odds ratio back to probability
  double base_odds = base_probability / (100 - base_probability);
  double new_odds = base_odds * odds_ratio;
  double probability = new_odds / (1 + new_odds);

  return probability;
}

void Person::determine_symptomatic_recrudescence(
    ClonalParasitePopulation* clinical_caused_parasite) {
  const auto random_p = Model::get_random()->random_flat(0.0, 1.0);

    /* Instead of getting prob. from the calculate_symptomatic_recrudescence_probability
     * which is depends on pfpr, use the one from immunity.
    */
    const auto pfpr = Model::get_mdc()->blood_slide_prevalence_by_location()[location_] * 100;

    const auto is_young_children = get_age() <= 6;

    const auto probability_develop_symptom =
        calculate_symptomatic_recrudescence_probability(pfpr, is_young_children);

    // if (random_p <= probability_develop_symptom) {

    /* UNCOMMENT THE LINE BELOW TO DISABLE THE SYMPTOMATIC RECRUDESCENCE */
    if (random_p <= get_probability_progress_to_clinical()) {

    // The last clinical caused parasite is going to relapse
    // regardless whether the induvidual are under treatment or not
    // Set the update function to progress to clinical
    clinical_caused_parasite->set_update_function(Model::progress_to_clinical_update_function());

    // Set the last update parasite density to the asymptomatic level

    clinical_caused_parasite->set_last_update_log10_parasite_density(
        Model::get_random()->random_normal_truncated(Model::get_config()
                                                         ->get_parasite_parameters()
                                                         .get_parasite_density_levels()
                                                         .get_log_parasite_density_asymptomatic(),
                                                     0.1));
    // clinical_caused_parasite->set_last_update_log10_parasite_density(
    //     Model::CONFIG->parasite_density_level()
    //         .log_parasite_density_asymptomatic);
    // Schedule a relapse event
    schedule_clinical_recurrence_event(clinical_caused_parasite);

    this->recurrence_status_ = Person::RecurrenceStatus::WITH_SYMPTOM;
    // mark the test treatment failure event as a failure
    for (auto &[time, event] : get_events()) {
      auto* tf_event = dynamic_cast<TestTreatmentFailureEvent*>(event.get());
      if (tf_event != nullptr && tf_event->clinical_caused_parasite() == clinical_caused_parasite) {
        event->set_executable(false);
        Model::get_mdc()->record_1_tf(location_, true);
        Model::get_mdc()->record_1_treatment_failure_by_therapy(location_, age_class_,
                                                                tf_event->therapy_id());
      }
    }

  } else {
    // continue the assymptomatic state with either having drug or immunity

    this->recurrence_status_ = Person::RecurrenceStatus::WITHOUT_SYMPTOM;

    // If the last update parasite density is greater than the asymptomatic
    // level, adjust it. We don't want to have high parasitaemia yn
    // asymptomatic
    if (clinical_caused_parasite->last_update_log10_parasite_density()
        > Model::get_config()
              ->get_parasite_parameters()
              .get_parasite_density_levels()
              .get_log_parasite_density_asymptomatic()) {
      clinical_caused_parasite->set_last_update_log10_parasite_density(
          Model::get_random()->random_normal_truncated(Model::get_config()
                                                           ->get_parasite_parameters()
                                                           .get_parasite_density_levels()
                                                           .get_log_parasite_density_asymptomatic(),
                                                       0.1));
    }

    if (drugs_in_blood_->size() > 0) {
      // Set the update function to having drug
      clinical_caused_parasite->set_update_function(Model::having_drug_update_function());
    } else {
      // Set the update function to immunity clearance
      clinical_caused_parasite->set_update_function(Model::immunity_clearance_update_function());
    }
  }
}

void Person::determine_clinical_or_not(ClonalParasitePopulation* clinical_caused_parasite) {
  if (all_clonal_parasite_populations_->contain(clinical_caused_parasite)) {
    // spdlog::info("Person::determine_clinical_or_not: Person has the parasite");
    const auto prob = Model::get_random()->random_flat(0.0, 1.0);
    if (prob <= get_probability_progress_to_clinical()) {
      // spdlog::info("Person::determine_clinical_or_not: Person will progress to clinical");
      // progress to clinical after several days
      clinical_caused_parasite->set_update_function(Model::progress_to_clinical_update_function());
      clinical_caused_parasite->set_last_update_log10_parasite_density(
          Model::get_config()
              ->get_parasite_parameters()
              .get_parasite_density_levels()
              .get_log_parasite_density_asymptomatic());
      // schedule_progress_to_clinical_event(clinical_caused_parasite);
      /* Old in V5 below (without recurence, schedule_relapse_event makes FOI match FOI in v5 */
      schedule_relapse_event(clinical_caused_parasite,
                             Model::get_config()->get_epidemiological_parameters().get_relapse_duration());
    } else {
      // spdlog::info("Person::determine_clinical_or_not: Person will progress to clearance");
      // progress to clearance
      clinical_caused_parasite->set_update_function(Model::immunity_clearance_update_function());
    }
  }
}

void Person::update() {
  // spdlog::info("Time: {}, Person::update, person age: {}",
  //              Model::get_scheduler()->current_time(), get_age());
  if (host_state_ == DEAD) {
    // throw an error
    spdlog::error("Person::update: Person is dead");
    throw std::runtime_error("Person is dead");
  }

  if (latest_update_time_ == Model::get_scheduler()->current_time()) return;

  // update parasites by immune system
  //    std::cout << "ppu"<< std::endl;
  // update the density of each blood parasite in parasite population
  // parasite will be killed by immune system
  all_clonal_parasite_populations_->update();

  // update all drugs concentration
  drugs_in_blood_->update();

  // update drug activity on parasite
  all_clonal_parasite_populations_->update_by_drugs(drugs_in_blood_.get());

  immune_system_->update();

  update_current_state();

  // update biting level only less than 1 to save performance
  //  the other will be update in birthday event
  update_relative_biting_rate();

  latest_update_time_ = Model::get_scheduler()->current_time();
  //    std::cout << "End Person Update"<< std::endl;
}

void Person::update_relative_biting_rate() {
  if (Model::get_config()
          ->get_epidemiological_parameters()
          .get_using_age_dependent_biting_level()) {
    current_relative_biting_rate_ =
        innate_relative_biting_rate_ * get_age_dependent_biting_factor();
  } else {
    current_relative_biting_rate_ = innate_relative_biting_rate_;
  }
}

void Person::update_current_state() {
  // clear drugs <=0.1
  drugs_in_blood_->clear_cut_off_drugs();
  // clear cured parasite
  all_clonal_parasite_populations_->clear_cured_parasites(Model::get_config()
                                                              ->get_parasite_parameters()
                                                              .get_parasite_density_levels()
                                                              .get_log_parasite_density_cured());

  if (all_clonal_parasite_populations_->size() == 0) {
    change_state_when_no_parasite_in_blood();
  } else {
    immune_system_->set_increase(true);
  }
}

void Person::randomly_choose_parasite() {
  if (today_infections_.empty()) {
    // already chose
    return;
  }
  if (today_infections_.size() == 1) {
    infected_by(today_infections_.at(0));
  } else {
    const std::size_t index_random_parasite =
        Model::get_random()->random_uniform(today_infections_.size());
    infected_by(today_infections_.at(index_random_parasite));
  }

  today_infections_.clear();
}

void Person::infected_by(const int &parasite_type_id) {
  // only infect if liver is available :D
  if (liver_parasite_type_ == nullptr) {
    if (host_state_ == SUSCEPTIBLE) { set_host_state(EXPOSED); }

    Genotype* genotype = Model::get_genotype_db()->at(parasite_type_id);
    liver_parasite_type_ = genotype;

    // move parasite to blood in next 7 days
    schedule_move_parasite_to_blood(genotype, 7);
  }
}

/*
 * From Temple Malaria Simulation Person.cpp
 */

void Person::randomly_choose_target_location() {
  if (today_target_locations_.empty()) {
    // already chose
    return;
  }

  auto target_location{-1};
  if (today_target_locations_.size() == 1) {
    target_location = today_target_locations_.at(0);
  } else {
    const int index_random_location =
        static_cast<int>(Model::get_random()->random_uniform(today_target_locations_.size()));
    target_location = today_target_locations_.at(index_random_location);
  }

  schedule_move_to_target_location_next_day_event(target_location);

  today_target_locations_.clear();

#ifdef ENABLE_TRAVEL_TRACKING
  // Update the day of the last initiated trip to the next day from current
  // time.
  day_that_last_trip_was_initiated_ = Model::get_scheduler()->current_time() + 1;

  // Check for district raster data availability for spatial analysis.
  if (Model::get_spatial_data()->has_raster(SpatialData::SpatialFileType::Districts)) {
    auto &spatial_data = Model::get_spatial_data();

    // Determine the source and destination districts for the current trip.
    int source_district = spatial_data.get_district(location_);
    int destination_district = spatial_data.get_district(target_location);

    // If the trip crosses district boundaries, update the day of the last
    // outside-district trip to the next day from current time.
    if (source_district != destination_district) {
      day_that_last_trip_outside_district_was_initiated_ =
          Model::get_scheduler()->current_time() + 1;
    }
    /* fmt::print("Person {} moved from district {} to district {}\n", _uid, */
    /*            source_district, destination_district); */
  }
#endif
}

bool Person::has_return_to_residence_event() const { return has_event<ReturnToResidenceEvent>(); }

void Person::cancel_all_return_to_residence_events() {
  cancel_all_events<ReturnToResidenceEvent>();
}

bool Person::has_detectable_parasite() const {
  auto detectable_threshold = Model::get_config()
                                  ->get_parasite_parameters()
                                  .get_parasite_density_levels()
                                  .get_log_parasite_density_detectable_pfpr();
  return all_clonal_parasite_populations_->has_detectable_parasite(detectable_threshold);
}

void Person::increase_number_of_times_bitten() {
  if (Model::get_scheduler()->current_time()
      >= Model::get_config()->get_simulation_timeframe().get_start_collect_data_day()) {
    number_of_times_bitten_++;
  }
}

double Person::get_age_dependent_biting_factor() const {
  //
  // 0.00 - 0.25  -  6.5
  // 0.25 - 0.50  -  8.0
  // 0.50 - 0.75  -  9.0
  // 0.75 - 1.00  -  9.5
  // 1.00 - 2.00  -  11.0
  // 2.00 - 3.00  -  13.5
  // 3.00 - 4.00  -  15.5
  // 4.00 - 5.00  -  17.5
  // + 2.75kg until 20
  // then divide by 61.5

  if (age_ < 1) {
    const auto age =
        ((Model::get_scheduler()->current_time() - birthday_) % Constants::DAYS_IN_YEAR)
        / static_cast<double>(Constants::DAYS_IN_YEAR);
    if (age < 0.25) return 0.106;
    if (age < 0.5) return 0.13;
    if (age < 0.75) return 0.1463;
    return 0.1545;
  }
  if (age_ < 2) return 0.1789;
  if (age_ < 3) return 0.2195;
  if (age_ < 4) return 0.2520;
  if (age_ < 20) return (17.5 + (age_ - 4) * 2.75) / 61.5;
  return 1.0;
}

bool Person::is_gametocytaemic() const {
  return all_clonal_parasite_populations_->is_gametocytaemic();
}

void Person::generate_prob_present_at_mda_by_age() {
  if (get_prob_present_at_mda_by_age().empty()) {
    for (auto i = 0; i < Model::get_config()
                             ->get_strategy_parameters()
                             .get_mda()
                             .get_mean_prob_individual_present_at_mda()
                             .size();
         i++) {
      auto value =
          Model::get_random()->random_beta(Model::get_config()
                                               ->get_strategy_parameters()
                                               .get_mda()
                                               .get_prob_individual_present_at_mda_distribution()[i]
                                               .alpha,
                                           Model::get_config()
                                               ->get_strategy_parameters()
                                               .get_mda()
                                               .get_prob_individual_present_at_mda_distribution()[i]
                                               .beta);
      prob_present_at_mda_by_age_.push_back(value);
    }
  }
}

double Person::prob_present_at_mda() {
  auto mda_age_index = 0;
  // std::cout << "hello " << i << std::endl;
  while (age_ > Model::get_config()
                    ->get_strategy_parameters()
                    .get_mda()
                    .get_age_bracket_prob_individual_present_at_mda()[mda_age_index]
         && mda_age_index < Model::get_config()
                                ->get_strategy_parameters()
                                .get_mda()
                                .get_age_bracket_prob_individual_present_at_mda()
                                .size()) {
    mda_age_index++;
  }

  return prob_present_at_mda_by_age_[mda_age_index];
}

bool Person::has_effective_drug_in_blood() const {
  for (const auto &kv_drug : *drugs_in_blood_) {
    if (kv_drug.second->last_update_value() > 0.5) return true;
  }
  return false;
}

bool Person::has_birthday_event() const { return has_event<BirthdayEvent>(); }

bool Person::has_update_by_having_drug_event() const {
  return has_event<UpdateWhenDrugIsPresentEvent>();
}

double Person::p_infection_from_an_infectious_bite() const {
  return ((1 - immune_system_->get_current_value()) / 8.333) + 0.04;
}

double Person::draw_random_relative_biting_rate(utils::Random* p_random, Config* p_config) {
  auto result = p_random->random_gamma(p_config->get_epidemiological_parameters().gamma_a,
                                       p_config->get_epidemiological_parameters().gamma_b);

  while (result > (p_config->get_epidemiological_parameters()
                       .get_relative_biting_info()
                       .get_max_relative_biting_value()
                   - p_config->get_epidemiological_parameters()
                         .get_relative_biting_info()
                         .get_min_relative_biting_value())) {
    // re-draw
    result = p_random->random_gamma(p_config->get_epidemiological_parameters().gamma_a,
                                    p_config->get_epidemiological_parameters().gamma_b);
  }

  return result
         + p_config->get_epidemiological_parameters()
               .get_relative_biting_info()
               .get_min_relative_biting_value();
}

double Person::age_in_floating(int simulation_time) const {
  auto days = simulation_time - birthday_;
  return days / static_cast<double>(Constants::DAYS_IN_YEAR);
}

/*
 * NEW KIEN
 */

void Person::increase_age_by_1_year() { set_age(age_ + 1); }

PersonEvent* Person::schedule_basic_event(std::unique_ptr<PersonEvent> event) {
  event->set_person(this);
  if (event->get_time() < Model::get_scheduler()->current_time()) {
    spdlog::error("Event time is less than current time {} < {}",
      event->get_time(), Model::get_scheduler()->current_time());
    throw std::invalid_argument("Event time is less than current time");
  }

  // if (event->get_time() > Model::get_config()->get_simulation_timeframe().get_total_time()) {
  //   spdlog::warn("Event time is greater than total time, event will not be scheduled. Event name:
  //   {}", event->name());
  //   // delete the event
  //   event.release();
  //   return;
  // }

  // Simply allow event to be scheduled even if it's time is greater than total time

  // schedule and transfer ownership of the event to the event_manager
  event_manager_.schedule_event(std::move(event));
  return event_manager_.get_events().begin()->second.get();
}

void Person::schedule_update_by_drug_event(ClonalParasitePopulation* parasite) {
  auto event = std::make_unique<UpdateWhenDrugIsPresentEvent>(this);
  event->set_time(calculate_future_time(1));
  event->set_clinical_caused_parasite(parasite);
  schedule_basic_event(std::move(event));
}

void Person::schedule_end_clinical_event(ClonalParasitePopulation* parasite) {
  // Clinical duration is normally distributed between 5-14 days, centered at 7
  int clinical_duration = Model::get_random()->random_normal_int(7, 2);
  clinical_duration = std::min(std::max(clinical_duration, 5), 14);

  auto event = std::make_unique<EndClinicalEvent>(this);
  event->set_time(calculate_future_time(clinical_duration));
  event->set_clinical_caused_parasite(parasite);
  schedule_basic_event(std::move(event));
}

void Person::schedule_progress_to_clinical_event(ClonalParasitePopulation* parasite) {
  // Time to clinical varies by age
  const int days_to_clinical =
      (age_ <= 5)
          ? Model::get_config()->get_epidemiological_parameters().get_days_to_clinical_under_five()
          : Model::get_config()->get_epidemiological_parameters().get_days_to_clinical_over_five();

  auto event = std::make_unique<ProgressToClinicalEvent>(this);
  event->set_time(calculate_future_time(days_to_clinical));
  event->set_clinical_caused_parasite(parasite);
  schedule_basic_event(std::move(event));
}

// void Person::schedule_clinical_recurrence_event(ClonalParasitePopulation* parasite) {
//   // assumming the onset of clinical symptoms is day 14 to 63 and end of
//   // clinical symptom is day 7.
//   // Clinical recurrence occurs between days 7-54, normally distributed around day 14
//   int days_to_clinical = Model::get_random()->random_normal_int(14, 5);
//   days_to_clinical = std::min(std::max(days_to_clinical, 7), 54);
//
//   auto event = std::make_unique<ProgressToClinicalEvent>(this);
//   event->set_time(calculate_future_time(days_to_clinical));
//   event->set_clinical_caused_parasite(parasite);
//   schedule_basic_event(std::move(event));
// }

void Person::schedule_clinical_recurrence_event(ClonalParasitePopulation* parasite) {
  // Clinical recurrence occurs between days 7-54, normally distributed around day 14
  int days_to_clinical = Model::get_random()->random_normal_int(14, 5);
  days_to_clinical = std::min(std::max(days_to_clinical, 7), 54);

  int new_event_time = calculate_future_time(days_to_clinical);

  // Log if similar event is already scheduled within ±7 days
  for (const auto& [time, existing_event] : get_events()) {
    auto* end_clinical_event = dynamic_cast<EndClinicalEvent*>(existing_event.get());
    if (end_clinical_event != nullptr) {
      int end_clinical_existing_time = end_clinical_event->get_time();
      if (new_event_time <= end_clinical_existing_time) {
        spdlog::info(
            "Model time {}, schedule recurrence event at time {}, clinical end event at time {}",
            Model::get_scheduler()->current_time(),new_event_time,end_clinical_existing_time);
      }
    }
    auto* existing_progress_event = dynamic_cast<ProgressToClinicalEvent*>(existing_event.get());
    if (existing_progress_event != nullptr) {
      int existing_time = existing_progress_event->get_time();
      if (std::abs(existing_time - new_event_time) <= 7
        && (new_event_time != existing_time)
        && existing_progress_event->is_executable()) {
        Model::get_mdc()->progress_to_clinical_in_7d_counter[location_].total++;
        if (existing_progress_event->clinical_caused_parasite() == parasite) {
          Model::get_mdc()->progress_to_clinical_in_7d_counter[location_].recrudescence++;
        }
        else {
          Model::get_mdc()->progress_to_clinical_in_7d_counter[location_].new_infection++;
        }
      }
    }
  }
  // Schedule the new event
  auto event = std::make_unique<ProgressToClinicalEvent>(this);
  event->set_time(new_event_time);
  event->set_clinical_caused_parasite(parasite);
  schedule_basic_event(std::move(event));
}


void Person::schedule_test_treatment_failure_event(ClonalParasitePopulation* parasite,
                                                   int testing_day, int therapy_id) {
  auto event = std::make_unique<TestTreatmentFailureEvent>(this);
  event->set_time(calculate_future_time(testing_day));
  event->set_therapy_id(therapy_id);
  event->set_clinical_caused_parasite(parasite);
  schedule_basic_event(std::move(event));
}

void Person::schedule_report_treatment_failure_death_event(int therapy_id, int testing_day) {
  auto event = std::make_unique<ReportTreatmentFailureDeathEvent>(this);
  event->set_time(calculate_future_time(testing_day));
  event->set_therapy_id(therapy_id);
  event->set_age_class(age_class_);
  event->set_location_id(location_);
  schedule_basic_event(std::move(event));
}

void Person::schedule_rapt_event(int days_delay) {
  auto event = std::make_unique<RaptEvent>(this);
  event->set_time(calculate_future_time(days_delay));
  schedule_basic_event(std::move(event));
}

void Person::schedule_receive_mda_therapy_event(Therapy* therapy, int days_delay) {
  auto event = std::make_unique<ReceiveMDATherapyEvent>(this);
  event->set_time(calculate_future_time(days_delay));
  event->set_received_therapy(therapy);
  schedule_basic_event(std::move(event));
}

void Person::schedule_receive_therapy_event(ClonalParasitePopulation* parasite, Therapy* therapy,
                                            int days_delay, bool is_part_of_mac_therapy) {
  auto event = std::make_unique<ReceiveTherapyEvent>(this);
  event->set_time(calculate_future_time(days_delay));
  event->set_clinical_caused_parasite(parasite);
  event->set_received_therapy(therapy);
  event->set_is_part_of_mac_therapy(is_part_of_mac_therapy);
  schedule_basic_event(std::move(event));
}

void Person::schedule_switch_immune_component_event(int days_delay) {
  auto event = std::make_unique<SwitchImmuneComponentEvent>(this);
  event->set_time(calculate_future_time(days_delay));
  schedule_basic_event(std::move(event));
}

void Person::schedule_move_parasite_to_blood(Genotype* genotype, int days_delay) {
  auto event = std::make_unique<MoveParasiteToBloodEvent>(this);
  event->set_time(calculate_future_time(days_delay));
  event->set_infection_genotype(genotype);
  schedule_basic_event(std::move(event));
}

void Person::schedule_mature_gametocyte_event(ClonalParasitePopulation* parasite) {
  const int days_to_mature = (age_ <= 5) ? Model::get_config()
                                               ->get_epidemiological_parameters()
                                               .get_days_mature_gametocyte_under_five()
                                         : Model::get_config()
                                               ->get_epidemiological_parameters()
                                               .get_days_mature_gametocyte_over_five();

  auto event = std::make_unique<MatureGametocyteEvent>(this);
  event->set_time(calculate_future_time(days_to_mature));
  event->set_blood_parasite(parasite);
  schedule_basic_event(std::move(event));
}

void Person::schedule_move_to_target_location_next_day_event(int target_location) {
  this->number_of_trips_taken_++;

  auto event = std::make_unique<CirculateToTargetLocationNextDayEvent>(this);
  event->set_time(calculate_future_time(1));
  event->set_target_location(target_location);
  schedule_basic_event(std::move(event));
}

void Person::schedule_return_to_residence_event(int length_of_trip) {
  auto event = std::make_unique<ReturnToResidenceEvent>(this);
  event->set_time(calculate_future_time(length_of_trip));
  schedule_basic_event(std::move(event));
}

void Person::schedule_birthday_event(int days_to_next_birthday) {
  if (days_to_next_birthday <= 0) {
    throw std::invalid_argument("days_to_next_birthday must be greater than 0");
  }
  auto event = std::make_unique<BirthdayEvent>(this);
  event->set_time(calculate_future_time(days_to_next_birthday));
  schedule_basic_event(std::move(event));
}

int Person::calculate_future_time(int days_from_now) {
  return Model::get_scheduler()->current_time() + days_from_now;
}

void Person::schedule_relapse_event(ClonalParasitePopulation* clinical_caused_parasite, const int& time_until_relapse) {
  int duration = Model::get_random()->random_normal(time_until_relapse, 15);
  duration = std::min<int>(std::max<int>(duration, time_until_relapse - 15), time_until_relapse + 15);
  auto event = std::make_unique<ProgressToClinicalEvent>(this);
  event->set_clinical_caused_parasite(clinical_caused_parasite);
  event->set_time(Model::get_scheduler()->current_time() + duration);
  schedule_basic_event(std::move(event));
}

void Person::determine_relapse_or_not(ClonalParasitePopulation* clinical_caused_parasite) {
  if (all_clonal_parasite_populations_->contain(clinical_caused_parasite)) {
    const auto p = Model::get_random()->random_flat(0.0, 1.0);

    if (p <= Model::get_config()->get_epidemiological_parameters().get_p_relapse()) {
      //        if (P <= get_probability_progress_to_clinical()) {
      // progress to clinical after several days
      clinical_caused_parasite->set_update_function(Model::progress_to_clinical_update_function());
      // std::cout<<"\t\tPerson::determine_relapse_or_not relapse" << std::endl;
      clinical_caused_parasite->set_last_update_log10_parasite_density(
          Model::get_config()->get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_asymptomatic());
      schedule_relapse_event(clinical_caused_parasite, Model::get_config()->get_epidemiological_parameters().get_relapse_duration());

    } else {
      // progress to clearance
      if (clinical_caused_parasite->last_update_log10_parasite_density()
          > Model::get_config()->get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_asymptomatic()) {
        clinical_caused_parasite->set_last_update_log10_parasite_density(
            Model::get_config()->get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_asymptomatic());
          }
      clinical_caused_parasite->set_update_function(Model::immunity_clearance_update_function());
    }
  }
}

void Person::schedule_end_clinical_by_no_treatment_event(ClonalParasitePopulation* clinical_caused_parasite) {
  auto d_clinical = Model::get_random()->random_normal(7, 2);
  d_clinical = std::min<int>(std::max<int>(d_clinical, 5), 14);

  auto event = std::make_unique<EndClinicalByNoTreatmentEvent>(this);
  event->set_clinical_caused_parasite(clinical_caused_parasite);
  event->set_person(this);
  event->set_time(Model::get_scheduler()->current_time() + d_clinical);
  schedule_basic_event(std::move(event));
}
