#ifndef POPULATION_H
#define POPULATION_H

#include <cstddef>
#include <list>
#include <memory>
#include <vector>

#include "Person/Person.h"

using PersonIndexPtrList = std::list<std::unique_ptr<PersonIndex>>;

class Model;
class PersonIndexAll;
class PersonIndexByLocationStateAgeClass;
class PersonIndexByLocationBitingLevel;
class Population {
public:
  Population(Population &&) = delete;
  Population &operator=(Population &&) = delete;
  // Disable copy and assignment
  Population(const Population &) = delete;
  Population &operator=(const Population &) = delete;

  Population();

  virtual ~Population();

  void initialize();
  //
  // void update(int current_time);
  //
  // // Add a person to the population
  void add_person(std::unique_ptr<Person> person);
  //
  // // Remove a person from the population
  void remove_person(Person* person);

  /**
   * This function removes person pointer out of all of the person indexes
   * This will also delete the @person out of memory
   * @param person
   */
  virtual void remove_dead_person(Person* person);

  // /** Return the total number of individuals in the simulation. */
  // virtual std::size_t size();

  /** Return the total number of individuals in the given location. */
  virtual std::size_t size_at(const int &location);

  /**
   * Return the number of individuals in the population
   * If the input location is -1, return total size
   * @param location
   */
  std::size_t size(const int &location = -1, const int &age_class = core::K_INVALID_AGE_CLASS);

  virtual std::size_t size(const int &location, const Person::HostStates &hs, const int &age_class);

  std::size_t size_residents_only(const int &location);

  /**
   * Notify change of a particular person's property to all person indexes
   * @param p
   * @param property
   * @param oldValue
   * @param newValue
   */
  virtual void notify_change(Person* person,
                             const Person::Property &property,
                             const void* old_value,
                             const void* new_value);

  void perform_infection_event_at_location(int location, int tracking_index);

  void introduce_initial_cases();
  //
  void introduce_parasite(const int &location,
                          Genotype* parasite_type,
                          const int &num_of_infections);

  static void setup_initial_infection(Person* person, Genotype* parasite_type);

  void persist_force_of_infection_at_location(int location, int tracking_index);

  void perform_birth_event_at_location(int location);

  void perform_death_event_at_location(int location);

  void generate_individual(int location, int age_class);

  Person* give_1_birth(const int &location);

  void clear_dead_people_at_location(int location);

  [[nodiscard]] static IntVector prepare_circulation_context();

  void perform_circulation_from_location(int from_location, const IntVector &circulation_context);

  void perform_circulation_for_1_location(const int &from_location,
                                          const int &target_location,
                                          const int &number_of_circulations,
                                          std::vector<Person*> &today_circulations);

  bool has_0_case();

  void initialize_person_indices();

  void prepare_daily_state_at_location(int location);

  void execute_all_individual_events(int up_to_time);

  void update_current_foi();

  // Notify the population that a person has moved from the source location, to
  // the destination location
  void notify_movement(int source, int destination);

  PersonIndexPtrList* person_index_list() { return person_index_list_.get(); }
  PersonIndexAll* all_persons() { return all_persons_.get(); }

  template <typename T>
  T* get_person_index();

  IntVector get_popsize_by_location() { return popsize_by_location_; }
  void set_popsize_by_location(const IntVector &popsize_by_location) {
    popsize_by_location_ = popsize_by_location;
  }

  [[nodiscard]] std::vector<std::vector<double>> &individual_foi_by_location() {
    return individual_foi_by_location_;
  }

  [[nodiscard]] std::vector<std::vector<double>> &individual_relative_biting_by_location() {
    return individual_relative_biting_by_location_;
  }

  [[nodiscard]] std::vector<std::vector<double>> &individual_relative_moving_by_location() {
    return individual_relative_moving_by_location_;
  }

  [[nodiscard]] std::vector<double> &sum_relative_biting_by_location() {
    return sum_relative_biting_by_location_;
  }

  [[nodiscard]] std::vector<double> &sum_relative_moving_by_location() {
    return sum_relative_moving_by_location_;
  }

  [[nodiscard]] std::vector<double> &current_force_of_infection_by_location() {
    return current_force_of_infection_by_location_;
  }

  [[nodiscard]] std::vector<std::vector<double>> &force_of_infection_for_n_days_by_location() {
    return force_of_infection_for_n_days_by_location_;
  }

  [[nodiscard]] std::vector<std::vector<Person*>> &all_alive_persons_by_location() {
    return all_alive_persons_by_location_;
  }

private:
  void reset_daily_sampling_state(int location);

  void append_daily_sampling_state(int location, Person* person);

  void remove_from_daily_sampling_state(int location, Person* person);

  void update_people_and_append_sampling_state(int location);

  /**
   * Calculate the challenge-mode infection probability for one person.
   *
   * Immunity at or below 0.2 leaves the configured transmission probability
   * unchanged. Immunity at or above 0.8 uses a probability of 0.1. Values
   * between those thresholds are linearly interpolated.
   */
  [[nodiscard]] static double calculate_challenge_infection_probability(
      Person &person, double transmission_probability);

  /** Draw once and determine whether an infectious bite causes infection. */
  [[nodiscard]] static bool is_infection_caused_by_bite(Person &person,
                                                        bool use_sporozoite_challenge);

  /** Return whether the person has an available liver stage for a new infection. */
  [[nodiscard]] static bool can_receive_new_infection(Person &person);

  /**
   * Process one sampled bite and retain its genotype when infection succeeds.
   *
   * A person may receive multiple bites and therefore accumulate multiple
   * candidate genotypes, but is inserted into infected_people only once.
   */
  static void process_bite(Person &person,
                           int location,
                           int tracking_index,
                           bool use_sporozoite_challenge,
                           PersonPtrVector &infected_people);

  /** Generate and process today's bites for a single location. */
  void process_infections_at_location(int location,
                                      int tracking_index,
                                      bool use_sporozoite_challenge,
                                      PersonPtrVector &infected_people);

  /** Record and choose one candidate genotype for each newly infected person. */
  static void finalize_today_infections(const PersonPtrVector &infected_people);

  std::unique_ptr<PersonIndexAll> all_persons_{nullptr};

  std::unique_ptr<PersonIndexPtrList> person_index_list_{nullptr};
  IntVector popsize_by_location_;

  std::vector<std::vector<double>> individual_foi_by_location_;
  std::vector<std::vector<double>> individual_relative_biting_by_location_;
  std::vector<std::vector<double>> individual_relative_moving_by_location_;

  std::vector<double> sum_relative_biting_by_location_;
  std::vector<double> sum_relative_moving_by_location_;

  std::vector<double> current_force_of_infection_by_location_;
  std::vector<std::vector<double>> force_of_infection_for_n_days_by_location_;
  std::vector<std::vector<Person*>> all_alive_persons_by_location_;
};

template <typename T>
T* Population::get_person_index() {
  for (auto &person_index_ptr : *person_index_list_) {
    T* pi = dynamic_cast<T*>(person_index_ptr.get());
    if (pi != nullptr) { return pi; }
  }
  return nullptr;
}
#endif  // POPULATION_H
