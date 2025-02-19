#ifndef PERSON_H
#define PERSON_H

#include <queue>
#include <vector>
#include <memory>
#include <uuid.h>
#include <Core/Scheduler/Dispatcher.h>
#include "Events/Event.h"
#include "date/date.h"

namespace utils {
    class Random;
}

class Config;
class Population;
class Event;
class Dispatcher;

class Person : public Dispatcher{
    // Disable copy and assignment
    Person(const Person&) = delete;
    Person& operator=(const Person&) = delete;
    Person(Person&&) = delete;
    Person& operator=(Person&&) = delete;

public:
    enum Property {
        LOCATION = 0,
        HOST_STATE,
        AGE,
        AGE_CLASS,
        BITTING_LEVEL,
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
    int lastest_update_time_{};
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
    // ImmuneSystem *immune_system_{};
    // SingleHostClonalParasitePopulations *all_clonal_parasite_populations_{};
    // DrugsInBlood *drugs_in_blood_{};

public:
    void initialize();

    // Method to run events before a certain time
    void execute_events(int time);

    // Method to add an event
    void add_event(Event* event);

    // Method to remove an event
    void remove_event(Event* event);

    void increase_age_by_1_year();

    [[nodiscard]] uuids::uuid get_id() const { return id_; }
    [[nodiscard]] std::string get_id_str() const { return to_string(id_).substr(to_string(id_).length()-8, 8); }

    void update(int time);

    Population* get_population() const;
    void set_population(Population* population);

    int get_location() const;
    void set_location(int location);

    int get_residence_location() const;
    void set_residence_location(int residence_location);

    HostStates get_host_state() const;
    void set_host_state(HostStates host_state);

    int get_age_class() const;
    void set_age_class(int age_class);

    int get_birthday() const;
    void set_birthday(int birthday);

    int get_lastest_update_time() const;
    void set_lastest_update_time(int lastest_update_time);

    int get_moving_level() const;
    void set_moving_level(int moving_level);

    std::vector<int>* get_today_infections() const;
    void set_today_infections(std::vector<int>* today_infections);

    std::vector<int>* get_today_target_locations() const;
    void set_today_target_locations(std::vector<int>* today_target_locations);

    std::vector<double> get_prob_present_at_mda_by_age() const;
    void set_prob_present_at_mda_by_age(const std::vector<double>& prob_present_at_mda_by_age);

    int get_number_of_times_bitten() const;
    void set_number_of_times_bitten(int number_of_times_bitten);

    int get_number_of_trips_taken() const;
    void set_number_of_trips_taken(int number_of_trips_taken);

    int get_last_therapy_id() const;
    void set_last_therapy_id(int last_therapy_id);

    std::map<int, double> get_starting_drug_values_for_MAC() const;
    void set_starting_drug_values_for_MAC(const std::map<int, double>& starting_drug_values_for_MAC);

    double get_innate_relative_biting_rate() const;
    void set_innate_relative_biting_rate(double innate_relative_biting_rate);

    double get_current_relative_biting_rate() const;
    void set_current_relative_biting_rate(double current_relative_biting_rate);

    void NotifyChange(const Property &property, const void *oldValue, const void *newValue);

  // ClonalParasitePopulation *add_new_parasite_to_blood(Genotype *parasite_type) const;

  static double relative_infectivity(const double &log10_parasite_density);

  // virtual double get_probability_progress_to_clinical();
  //
  // virtual bool will_progress_to_death_when_receive_no_treatment();
  //
  // virtual bool will_progress_to_death_when_recieve_treatment();

  void cancel_all_other_progress_to_clinical_events_except(Event *event) const;

  void cancel_all_events_except(Event *event) const;

  //    void record_treatment_failure_for_test_treatment_failure_events();
  //
  // void change_all_parasite_update_function(ParasiteDensityUpdateFunction *from,
  //                                          ParasiteDensityUpdateFunction *to) const;

  int complied_dosing_days(const int &dosing_day) const;
  //
  // void receive_therapy(Therapy *therapy, ClonalParasitePopulation *clinical_caused_parasite,
  //                      bool is_part_of_MAC_therapy = false);
  //
  // void add_drug_to_blood(DrugType *dt, const int &dosing_days, bool is_part_of_MAC_therapy = false);
  //
  // void schedule_progress_to_clinical_event_by(ClonalParasitePopulation *blood_parasite);
  //
  // void schedule_test_treatment_failure_event(ClonalParasitePopulation *blood_parasite, const int &testing_day,
  //                                            const int &t_id = 0);
  //
  // void schedule_update_by_drug_event(ClonalParasitePopulation *clinical_caused_parasite);
  //
  // void schedule_end_clinical_event(ClonalParasitePopulation *clinical_caused_parasite);
  //
  // void schedule_end_clinical_by_no_treatment_event(ClonalParasitePopulation *clinical_caused_parasite);
  //
  // void schedule_relapse_event(ClonalParasitePopulation *clinical_caused_parasite, const int &time_until_relapse);
  //
  // void schedule_move_parasite_to_blood(Genotype *genotype, const int &time);
  //
  // void schedule_mature_gametocyte_event(ClonalParasitePopulation *clinical_caused_parasite);
  //
  // void change_state_when_no_parasite_in_blood();
  //
  // void determine_relapse_or_not(ClonalParasitePopulation *clinical_caused_parasite);
  //
  // void determine_clinical_or_not(ClonalParasitePopulation *clinical_caused_parasite);

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

  void update_relative_bitting_rate();

  double p_infection_from_an_infectious_bite() const;

  bool isGametocytaemic() const;

  void generate_prob_present_at_mda_by_age();

  double prob_present_at_mda();

  bool has_effective_drug_in_blood() const;

  static double draw_random_relative_biting_rate(utils::Random *pRandom, Config *pConfig);

};

#endif //PERSON_H