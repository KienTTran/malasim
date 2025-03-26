#ifndef PERSON_H
#define PERSON_H

#include <queue>
#include <vector>
#include <memory>
#include <uuid.h>
#include <Core/Scheduler/Dispatcher.h>
#include "Events/Event.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Utils/Index/PersonIndexAllHandler.h"
#include "Utils/Index/PersonIndexByLocationMovingLevelHandler.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClassHandler.h"
#include "date/date.h"

class SCTherapy;

namespace utils {
    class Random;
}

class Config;
class Population;
class Event;
class Dispatcher;
class ImmuneSystem;

class Person :  public PersonIndexAllHandler,
                public PersonIndexByLocationStateAgeClassHandler,
                public PersonIndexByLocationMovingLevelHandler,
                public Dispatcher {
  // OBJECTPOOL(Person)

  // Disable copy and assignment and move
  Person(const Person&) = delete;
  void operator=(const Person&) = delete;
  Person(Person&&) = delete;
  Person& operator=(Person&&) = delete;

public:
  enum Property {
    LOCATION = 0,
    HOST_STATE,
    AGE,
    AGE_CLASS,
    BITING_LEVEL,
    MOVING_LEVEL,
    EXTERNAL_POPULATION_MOVING_LEVEL
  };

  enum HostStates { SUSCEPTIBLE = 0, EXPOSED = 1, ASYMPTOMATIC = 2, CLINICAL = 3, DEAD = 4, NUMBER_OF_STATE = 5 };
  // Comparison function for priority queue
  struct EventComparator {
    bool operator()(Event* lhs, Event* rhs) const {
      return lhs->time > rhs->time;
    }
  };

  // Priority queue of Event
  std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;

  Person();

  ~Person();
private:
  int age_;
  uuids::uuid id_;
  Population* population_{};
  int location_{};
  int residence_location_{};
  HostStates host_state_;
  int age_class_{};
  int birthday_{};
  int latest_update_time_{};
  int moving_level_{};
  std::vector<int> *today_infections_{};
  std::vector<int> *today_target_locations_{};
  std::vector<double> prob_present_at_mda_by_age_;
  int number_of_times_bitten_{};
  int number_of_trips_taken_{};
  int last_therapy_id_{};
  std::map<int, double> starting_drug_values_for_MAC_;
  double innate_relative_biting_rate_;
  double current_relative_biting_rate_;
  ImmuneSystem *immune_system_{};
  SingleHostClonalParasitePopulations *all_clonal_parasite_populations_{};
  DrugsInBlood *drugs_in_blood_{};
  Genotype *liver_parasite_type_{};
#ifdef ENABLE_TRAVEL_TRACKING
  int day_that_last_trip_was_initiated_{-1};
  int day_that_last_trip_outside_district_was_initiated_{-1};
public:
  int get_day_that_last_trip_was_initiated() const { return day_that_last_trip_was_initiated_; }
  int get_day_that_last_trip_outside_district_was_initiated() const { return day_that_last_trip_outside_district_was_initiated_; }
#endif

public:
    void initialize();

    // // Method to run events before a certain time
    void execute_events(int time);

    // Method to add an event
    void add_event(Event* event);

    // Method to remove an event
    void remove_event(Event* event);

    void increase_age_by_1_year();

    [[nodiscard]] uuids::uuid get_id_raw() const { return id_; }
    [[nodiscard]] std::string get_id() const { return to_string(id_).substr(to_string(id_).length()-8, 8); }

    void update();
    void update_events(int time);
    
    Population* get_population() const { return population_; }
    void set_population(Population* population) { population_ = population; }

    void set_age_class(const int& value);

    void set_moving_level(const int& value);

    ImmuneSystem* get_immune_system() {
        return immune_system_;
    }

    SingleHostClonalParasitePopulations* get_all_clonal_parasite_populations() {
      return all_clonal_parasite_populations_;
    }
    void set_all_clonal_parasite_populations(SingleHostClonalParasitePopulations* all_clonal_parasite_populations) {
      all_clonal_parasite_populations_ = all_clonal_parasite_populations;
    }

    void set_immune_system(ImmuneSystem* value);

    int get_location() const { return location_; }

    int get_residence_location() const { return residence_location_; }

    void set_residence_location(int residence_location) { residence_location_ = residence_location; }

    void set_host_state(const HostStates& value);

    Person::HostStates get_host_state() const { return host_state_; }

    void set_age(const int& value);
    int get_age() const { return age_; }

    int get_age_class() const { return age_class_; }

    int get_birthday() const { return birthday_; }
    void set_birthday(int birthday) { birthday_ = birthday; }

    int get_latest_update_time() const { return latest_update_time_; }
    void set_latest_update_time(int lastest_update_time) { latest_update_time_ = lastest_update_time; }

    int get_moving_level() const { return moving_level_; }

    std::vector<int>* get_today_infections() const { return today_infections_; }
    void set_today_infections(std::vector<int>* today_infections) { today_infections_ = today_infections; }

    std::vector<int>* get_today_target_locations() const { return today_target_locations_; }
    void set_today_target_locations(std::vector<int>* today_target_locations) { today_target_locations_ = today_target_locations; }

    std::vector<double> get_prob_present_at_mda_by_age() const { return prob_present_at_mda_by_age_; }
    void set_prob_present_at_mda_by_age(const std::vector<double>& prob_present_at_mda_by_age) { prob_present_at_mda_by_age_ = prob_present_at_mda_by_age; }

    int get_number_of_times_bitten() const { return number_of_times_bitten_; }
    void set_number_of_times_bitten(int number_of_times_bitten) { number_of_times_bitten_ = number_of_times_bitten; }

    int get_number_of_trips_taken() const { return number_of_trips_taken_; }
    void set_number_of_trips_taken(int number_of_trips_taken) { number_of_trips_taken_ = number_of_trips_taken; }

    int get_last_therapy_id() const { return last_therapy_id_; }
    void set_last_therapy_id(int last_therapy_id) { last_therapy_id_ = last_therapy_id; }

    std::map<int, double> get_starting_drug_values_for_MAC() const { return starting_drug_values_for_MAC_; }
    void set_starting_drug_values_for_MAC(const std::map<int, double>& starting_drug_values_for_MAC) { starting_drug_values_for_MAC_ = starting_drug_values_for_MAC; }

    double get_innate_relative_biting_rate() const { return innate_relative_biting_rate_; }
    void set_innate_relative_biting_rate(double innate_relative_biting_rate) { innate_relative_biting_rate_ = innate_relative_biting_rate; }

    double get_current_relative_biting_rate() const { return current_relative_biting_rate_; }
    void set_current_relative_biting_rate(double current_relative_biting_rate) { current_relative_biting_rate_ = current_relative_biting_rate; }

    void update_relative_bitting_rate();

    void NotifyChange(const Property &property, const void *oldValue, const void *newValue);

    void set_location(const int& value);

    DrugsInBlood *drugs_in_blood() {
      return drugs_in_blood_;
    }
    void set_drugs_in_blood(DrugsInBlood *drugs_in_blood);

    Genotype* liver_parasite_type() {
      return liver_parasite_type_;
    }
    void set_liver_parasite_type(Genotype *liver_parasite_type) {
      liver_parasite_type_ = liver_parasite_type;
    }

  ClonalParasitePopulation *add_new_parasite_to_blood(Genotype *parasite_type) const;

  static double relative_infectivity(const double &log10_parasite_density);

  virtual double get_probability_progress_to_clinical();
  //
  virtual bool will_progress_to_death_when_receive_no_treatment();
  //
  virtual bool will_progress_to_death_when_recieve_treatment();

  void cancel_all_other_progress_to_clinical_events_except(Event *event) const;

  void cancel_all_events_except(Event *event) const;

  void change_all_parasite_update_function(ParasiteDensityUpdateFunction *from,
                                           ParasiteDensityUpdateFunction *to) const;

  int complied_dosing_days(const int &dosing_day) const;

  void receive_therapy(Therapy *therapy, ClonalParasitePopulation *clinical_caused_parasite,
                       bool is_part_of_MAC_therapy = false);

  void add_drug_to_blood(DrugType *dt, const int &dosing_days, bool is_part_of_MAC_therapy = false);

  void schedule_progress_to_clinical_event_by(ClonalParasitePopulation *blood_parasite);

  void schedule_test_treatment_failure_event(ClonalParasitePopulation *blood_parasite, const int &testing_day,
                                             const int &t_id = 0);

  void schedule_update_by_drug_event(ClonalParasitePopulation *clinical_caused_parasite);

  void schedule_end_clinical_event(ClonalParasitePopulation *clinical_caused_parasite);

  void schedule_end_clinical_by_no_treatment_event(ClonalParasitePopulation *clinical_caused_parasite);

  void schedule_relapse_event(ClonalParasitePopulation *clinical_caused_parasite, const int &time_until_relapse);

  // void schedule_move_parasite_to_blood(Genotype *genotype, const int &time);
  //
  void schedule_mature_gametocyte_event(ClonalParasitePopulation *clinical_caused_parasite);

  void change_state_when_no_parasite_in_blood();

  void determine_relapse_or_not(ClonalParasitePopulation *clinical_caused_parasite);

  void determine_clinical_or_not(ClonalParasitePopulation *clinical_caused_parasite);

  void update_current_state();

  void randomly_choose_parasite();

  void infected_by(const int &parasite_type_id);

  void randomly_choose_target_location();

  void schedule_move_to_target_location_next_day_event(const int &location);

  bool has_return_to_residence_event() const;

  void cancel_all_return_to_residence_events() const;

  bool has_detectable_parasite() const;

  void increase_number_of_times_bitten();

  void move_to_population(Population *target_population);

  bool has_birthday_event() const;

  bool has_update_by_having_drug_event() const;

  double get_age_dependent_biting_factor() const;

  void update_relative_biting_rate();

  double p_infection_from_an_infectious_bite() const;

  bool isGametocytaemic() const;

  void generate_prob_present_at_mda_by_age();

  double prob_present_at_mda();

  bool has_effective_drug_in_blood() const;

  static double draw_random_relative_biting_rate(utils::Random *pRandom, Config *pConfig);

  void schedule_move_parasite_to_blood(Genotype* genotype, const int& time);

  /*
   * From Temple Malaria Simulation
  */

  void receive_therapy(SCTherapy* sc_therapy, bool is_mac_therapy);

  int complied_dosing_days(const SCTherapy* therapy);

  double age_in_floating() const;

  void schedule_update_every_K_days_event(const int &time);
  // Check to see if the indicated event has been defined for the individual.
  template <typename T>
  bool has_event() const {
    for (auto event_pair : *events()) {
      if (dynamic_cast<T*>(event_pair.second) != nullptr && event_pair.second->executable
          && event_pair.second->dispatcher != nullptr) {
        return true;
          }
    }
    return false;
  }

};

#endif //PERSON_H