#ifndef PERSON_H
#define PERSON_H

#include <Core/Scheduler/EventManager.h>

#include <memory>
#include <unordered_set>
#include <vector>

#include "Events/Event.h"
#include "FollowupEpisodeTypes.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Utils/Index/PersonIndexAllHandler.h"
#include "Utils/Index/PersonIndexByLocationMovingLevelHandler.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClassHandler.h"

class SCTherapy;

namespace utils {
class Random;
}

class Config;
class Population;
class ImmuneSystem;

class Person : public PersonIndexAllHandler,
               public PersonIndexByLocationStateAgeClassHandler,
               public PersonIndexByLocationMovingLevelHandler {
  // OBJECTPOOL(Person)
public:
  // day_that_last_trip_outside_district_was_initiated_sable copy and assignment
  Person(const Person &) = delete;
  Person& operator=(const Person &) = delete;
  Person(Person &&) = delete;
  Person &operator=(Person &&) = delete;

  Person();
  ~Person() override;

  enum RecurrenceStatus : uint8_t { NONE = 0, WITHOUT_SYMPTOM = 1, WITH_SYMPTOM = 2 };

  enum Property : uint8_t {
    LOCATION = 0,
    HOST_STATE,
    AGE,
    AGE_CLASS,
    BITING_LEVEL,
    MOVING_LEVEL,
    EXTERNAL_POPULATION_MOVING_LEVEL
  };

  enum HostStates : uint8_t {
    SUSCEPTIBLE = 0,
    EXPOSED = 1,
    ASYMPTOMATIC = 2,
    CLINICAL = 3,
    DEAD = 4,
    NUMBER_OF_STATE = 5
  };

  void initialize();

  // Delegate event management methods to event_manager_
  PersonEvent* schedule_basic_event(std::unique_ptr<PersonEvent> event);
  void update_events(int time) { event_manager_.execute_events(time); }

  template <typename T>
  [[nodiscard]] bool has_event() const {
    return event_manager_.has_event<T>();
  }

  template <typename T>
  void cancel_all_events() {
    event_manager_.cancel_all_events<T>();
  }

  void cancel_all_events_except(PersonEvent* event) {
    event_manager_.cancel_all_events_except(event);
  }

  template <typename T>
  void cancel_all_events_except(PersonEvent* event) {
    event_manager_.cancel_all_events_except<T>(event);
  }

  std::multimap<int, std::unique_ptr<PersonEvent>> &get_events() {
    return event_manager_.get_events();
  }

  void increase_age_by_1_year();

  void update();

  [[nodiscard]] Population* get_population() const { return population_; }
  void set_population(Population* population) { population_ = population; }

  void set_age_class(const int &value);

  void set_moving_level(int value);

  ImmuneSystem* get_immune_system() { return immune_system_.get(); }

  // take ownership of immune system
  void set_immune_system(std::unique_ptr<ImmuneSystem> value);

  SingleHostClonalParasitePopulations* get_all_clonal_parasite_populations() {
    return all_clonal_parasite_populations_.get();
  }

  [[nodiscard]] int get_location() const { return location_; }

  [[nodiscard]] int get_residence_location() const { return residence_location_; }

  void set_residence_location(int residence_location) { residence_location_ = residence_location; }

  void set_host_state(const HostStates &value);

  [[nodiscard]] Person::HostStates get_host_state() const { return host_state_; }

  void set_age(const uint &value);
  [[nodiscard]] uint get_age() const { return age_; }

  [[nodiscard]] int get_age_class() const { return age_class_; }

  [[nodiscard]] int get_birthday() const { return birthday_; }
  void set_birthday(int birthday) { birthday_ = birthday; }

  [[nodiscard]] virtual int get_latest_update_time() const { return latest_update_time_; }
  virtual void set_latest_update_time(int lastest_update_time) {
    latest_update_time_ = lastest_update_time;
  }

  [[nodiscard]] int get_moving_level() const { return moving_level_; }

  std::vector<int> &get_today_infections() { return today_infections_; }

  std::vector<int> &get_today_target_locations() { return today_target_locations_; }

  [[nodiscard]] std::vector<double> get_prob_present_at_mda_by_age() const {
    return prob_present_at_mda_by_age_;
  }
  void set_prob_present_at_mda_by_age(const std::vector<double> &prob_present_at_mda_by_age) {
    prob_present_at_mda_by_age_ = prob_present_at_mda_by_age;
  }

  [[nodiscard]] int get_number_of_times_bitten() const { return number_of_times_bitten_; }
  void set_number_of_times_bitten(int number_of_times_bitten) {
    number_of_times_bitten_ = number_of_times_bitten;
  }

  [[nodiscard]] int get_number_of_trips_taken() const { return number_of_trips_taken_; }
  void set_number_of_trips_taken(int number_of_trips_taken) {
    number_of_trips_taken_ = number_of_trips_taken;
  }

  [[nodiscard]] int get_last_therapy_id() const { return last_therapy_id_; }
  void set_last_therapy_id(int last_therapy_id) { last_therapy_id_ = last_therapy_id; }

  [[nodiscard]] std::map<int, double> get_starting_drug_values_for_mac() const {
    return starting_drug_values_for_mac_;
  }
  void set_starting_drug_values_for_mac(const std::map<int, double> &starting_drug_values_for_mac) {
    starting_drug_values_for_mac_ = starting_drug_values_for_mac;
  }

  [[nodiscard]] double get_innate_relative_biting_rate() const {
    return innate_relative_biting_rate_;
  }
  void set_innate_relative_biting_rate(double innate_relative_biting_rate) {
    innate_relative_biting_rate_ = innate_relative_biting_rate;
  }

  [[nodiscard]] double get_current_relative_biting_rate() const {
    return current_relative_biting_rate_;
  }
  void set_current_relative_biting_rate(double current_relative_biting_rate) {
    current_relative_biting_rate_ = current_relative_biting_rate;
  }

  [[nodiscard]] int get_latest_time_received_public_treatment() const {
    return latest_time_received_public_treatment_;
  }
  void set_latest_time_received_public_treatment(int latest_time_received_public_treatment) {
    latest_time_received_public_treatment_ = latest_time_received_public_treatment;
  }

  [[nodiscard]] RecurrenceStatus get_recurrence_status() const { return recurrence_status_; }
  void set_recurrence_status(RecurrenceStatus recurrence_status) {
    recurrence_status_ = recurrence_status;
  }

  void update_relative_bitting_rate();

  void notify_change(const Property &property, const void* old_value, const void* new_value);

  void set_location(const int &value);

  DrugsInBlood* drugs_in_blood() { return drugs_in_blood_.get(); }

  Genotype* liver_parasite_type() { return liver_parasite_type_; }
  void set_liver_parasite_type(Genotype* liver_parasite_type) {
    liver_parasite_type_ = liver_parasite_type;
  }

  ClonalParasitePopulation* add_new_parasite_to_blood(Genotype* parasite_type) const;

  static double relative_infectivity(const double &log10_parasite_density);

  virtual double get_probability_progress_to_clinical();
  //
  virtual bool will_progress_to_death_when_receive_no_treatment();
  //
  virtual bool will_progress_to_death_when_recieve_treatment();

  void cancel_all_other_progress_to_clinical_events_except(PersonEvent* event);

  void change_all_parasite_update_function(ParasiteDensityUpdateFunction* from,
                                           ParasiteDensityUpdateFunction* to) const;

  void receive_therapy(Therapy* therapy, ClonalParasitePopulation* clinical_caused_parasite,
                       bool is_part_of_mac_therapy = false, bool is_public_sector = true);

  void add_drug_to_blood(DrugType* dt, const int &dosing_days, bool is_part_of_mac_therapy = false);

  void change_state_when_no_parasite_in_blood();

  void determine_symptomatic_recrudescence(ClonalParasitePopulation* clinical_caused_parasite);

  void determine_clinical_or_not(ClonalParasitePopulation* clinical_caused_parasite,
                                  FollowupEpisodeSourceHint source_hint = FollowupEpisodeSourceHint::ExistingHostParasite);

  void update_current_state();

  void randomly_choose_parasite();

  void infected_by(const int &parasite_type_id);

  void randomly_choose_target_location();

  [[nodiscard]] bool has_return_to_residence_event() const;

  void cancel_all_return_to_residence_events();

  [[nodiscard]] bool has_detectable_parasite() const;

  void increase_number_of_times_bitten();

  [[nodiscard]] bool has_birthday_event() const;

  [[nodiscard]] bool has_update_by_having_drug_event() const;

  [[nodiscard]] double get_age_dependent_biting_factor() const;

  void update_relative_biting_rate();

  [[nodiscard]] double p_infection_from_an_infectious_bite() const;

  [[nodiscard]] bool is_gametocytaemic() const;

  void generate_prob_present_at_mda_by_age();

  double prob_present_at_mda();

  [[nodiscard]] bool has_effective_drug_in_blood() const;

  static double draw_random_relative_biting_rate(utils::Random* p_random, Config* p_config);

  void receive_therapy(SCTherapy* sc_therapy, bool is_mac_therapy);

  [[nodiscard]] double age_in_floating(int simulation_time) const;

  // Group 1: Clinical Event Scheduling
  // void schedule_clinical_event(ClonalParasitePopulation* parasite, int days_delay);
  void schedule_end_clinical_event(ClonalParasitePopulation* parasite);
  void schedule_progress_to_clinical_event(
      ClonalParasitePopulation* parasite,
      FollowupEpisodeSourceHint source_hint = FollowupEpisodeSourceHint::ExistingHostParasite);
  void schedule_clinical_recurrence_event(ClonalParasitePopulation* parasite);
  void schedule_test_treatment_failure_event(ClonalParasitePopulation* parasite, int testing_day,
                                             int therapy_id = 0);
  void schedule_report_treatment_failure_death_event(int therapy_id, int testing_day);
  void schedule_rapt_event(int days_delay);
  void schedule_receive_mda_therapy_event(Therapy* therapy, int days_delay);
  void schedule_receive_therapy_event(ClonalParasitePopulation* parasite, Therapy* therapy,
                                      int days_delay, bool is_part_of_mac_therapy = false);
  void schedule_switch_immune_component_event(int days_delay);

  // Group 2: Parasite Event Scheduling
  void schedule_move_parasite_to_blood(Genotype* genotype, int days_delay);
  void schedule_mature_gametocyte_event(ClonalParasitePopulation* parasite);
  void schedule_update_by_drug_event(ClonalParasitePopulation* parasite);

  // Group 3: Movement Event Scheduling
  void schedule_move_to_target_location_next_day_event(int target_location);
  void schedule_return_to_residence_event(int length_of_trip);

  // Group 4: Person Event Scheduling
  void schedule_birthday_event(int days_to_next_birthday = -1);

  static int complied_dosing_days(const int &dosing_day);
  static int complied_dosing_days(const SCTherapy* therapy);
  // Helper methods for scheduling
  static int calculate_future_time(int days_from_now);

  void determine_relapse_or_not(ClonalParasitePopulation *clinical_caused_parasite);
  void schedule_relapse_event(ClonalParasitePopulation *clinical_caused_parasite, const int &time_until_relapse);

  void schedule_end_clinical_by_no_treatment_event(ClonalParasitePopulation *clinical_caused_parasite);

  // ---------------------------------------------------------------
  // 28-day follow-up tracking
  // ---------------------------------------------------------------
  struct PendingFollowupClinicalEvent {
    int event_day{-1};
    int location{-1};
    int age{-1};
    int age_class{-1};
    int therapy_id{-1};
    bool received_treatment{false};
    bool clinical_already_recorded{false};  // true when recorded immediately (outcome-known path)
    FollowupEpisodeSource classified_source_before_outcome_rule{FollowupEpisodeSource::Unknown};
    FollowupEpisodeSourceHint source_hint{FollowupEpisodeSourceHint::Unknown};  // scheduling-time hint, for final remap
  };

  void start_first_treatment_followup_window(int day,
                                             ClonalParasitePopulation* clinical_caused_parasite,
                                             int therapy_id);
  [[nodiscard]] bool has_active_first_treatment_followup_window() const;
  [[nodiscard]] bool is_in_first_treatment_28_day_window(int day) const;
  void add_pending_followup_event(const PendingFollowupClinicalEvent& event);
  // Flush pending events and reset the window (used at normal TF-test day).
  void flush_pending_followup_events_with_first_treatment_outcome_and_reset(
      FirstTreatmentOutcome outcome);
  // Mark outcome as known failure but keep window active (early recrudescence path).
  void mark_first_treatment_followup_outcome_known(FirstTreatmentOutcome outcome);
  // Flush pre-known-outcome pending events without resetting the window.
  void flush_pending_followup_events_with_known_outcome_and_keep_window(
      FirstTreatmentOutcome outcome);
  [[nodiscard]] bool first_treatment_followup_outcome_known() const {
    return first_treatment_followup_outcome_known_;
  }
  [[nodiscard]] FirstTreatmentOutcome first_treatment_followup_outcome() const {
    return first_treatment_followup_outcome_;
  }
  void reset_first_treatment_followup_window();
  void expire_first_treatment_followup_window_if_needed(int current_day);

  std::vector<PendingFollowupClinicalEvent>& get_pending_followup_events() {
    return pending_followup_events_;
  }

  [[nodiscard]] ClonalParasitePopulation* first_treatment_followup_parasite() const {
    return first_treatment_followup_parasite_;
  }
  [[nodiscard]] int first_treatment_followup_therapy_id() const {
    return first_treatment_followup_therapy_id_;
  }

  FollowupEpisodeSource classify_followup_episode_source(
      ClonalParasitePopulation* clinical_caused_parasite,
      FollowupEpisodeSourceHint source_hint) const;

  // Remaps source at flush time if outcome/hint combination is incompatible
  // (e.g. source_hint==Recrudescence but outcome==Success).
  static FollowupEpisodeSource remap_followup_source_for_outcome(
      FollowupEpisodeSource source,
      FollowupEpisodeSourceHint source_hint,
      FirstTreatmentOutcome outcome);

  // Returns true if clinical_caused_parasite matches the parasite that opened
  // the currently active first-treatment follow-up window (pointer equality).
  [[nodiscard]] bool recrudescence_belongs_to_active_first_treatment_window(
      ClonalParasitePopulation* clinical_caused_parasite) const;

  // Expose first_treatment_followup_day_ for debug logging (always available).
  [[nodiscard]] int first_treatment_followup_day() const {
    return first_treatment_followup_day_;
  }

  // ---------------------------------------------------------------
  // v9: Late recrudescence tracking
  // ---------------------------------------------------------------
  void register_treated_parasite(ClonalParasitePopulation* parasite);
  [[nodiscard]] bool was_parasite_previously_treated(ClonalParasitePopulation* parasite) const;
  void unregister_treated_parasite(ClonalParasitePopulation* parasite);
  void clear_treated_parasite_tracking();

private:
  int age_{-1};
  Population* population_{nullptr};
  int location_{-1};
  int residence_location_{-1};
  HostStates host_state_{SUSCEPTIBLE};
  int age_class_{-1};
  int birthday_{-1};
  int latest_update_time_{-1};
  int moving_level_{-1};
  std::vector<int> today_infections_;
  std::vector<int> today_target_locations_;
  std::vector<double> prob_present_at_mda_by_age_;
  int number_of_times_bitten_{0};
  int number_of_trips_taken_{0};
  int last_therapy_id_{0};
  std::map<int, double> starting_drug_values_for_mac_;
  double innate_relative_biting_rate_{0};
  double current_relative_biting_rate_{0};
  std::unique_ptr<ImmuneSystem> immune_system_{nullptr};
  std::unique_ptr<SingleHostClonalParasitePopulations> all_clonal_parasite_populations_{nullptr};
  std::unique_ptr<DrugsInBlood> drugs_in_blood_{nullptr};
  Genotype* liver_parasite_type_{nullptr};
  int latest_time_received_public_treatment_{-30};
  RecurrenceStatus recurrence_status_{RecurrenceStatus::NONE};
  EventManager<PersonEvent> event_manager_;

  // 28-day first-treatment follow-up window
  int first_treatment_followup_day_{-1};
  int first_treatment_followup_therapy_id_{-1};
  bool first_treatment_followup_active_{false};
  bool first_treatment_followup_outcome_known_{false};
  FirstTreatmentOutcome first_treatment_followup_outcome_{FirstTreatmentOutcome::Success};
  ClonalParasitePopulation* first_treatment_followup_parasite_{nullptr};
  std::vector<PendingFollowupClinicalEvent> pending_followup_events_;

  // v9: previously-treated parasite tracking
  std::unordered_set<ClonalParasitePopulation*> previously_treated_parasites_;

#ifdef ENABLE_TRAVEL_TRACKING
  int day_that_last_trip_was_initiated_{-1};
  int day_that_last_trip_outside_district_was_initiated_{-1};

public:
  int get_day_that_last_trip_was_initiated() const { return day_that_last_trip_was_initiated_; }
  int get_day_that_last_trip_outside_district_was_initiated() const {
    return day_that_last_trip_outside_district_was_initiated_;
  }
#endif
};

#endif  // PERSON_H

