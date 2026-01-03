#ifndef PROGRESSTOCLINICALEVENT_H
#define PROGRESSTOCLINICALEVENT_H

#include "Event.h"
// #include "Core/ObjectPool.h"
#include <string>

class Person;

class Scheduler;

class ClonalParasitePopulation;

class Therapy;

class ProgressToClinicalEvent : public PersonEvent {
  // OBJECTPOOL(ProgressToClinicalEvent)
public:
  // Disallow copy
  ProgressToClinicalEvent(const ProgressToClinicalEvent&) = delete;
  ProgressToClinicalEvent& operator=(const ProgressToClinicalEvent&) = delete;

  // Disallow move
  ProgressToClinicalEvent(ProgressToClinicalEvent&&) = delete;
  ProgressToClinicalEvent& operator=(ProgressToClinicalEvent&&) = delete;

  explicit ProgressToClinicalEvent(Person* person) : PersonEvent(person) {}

  ~ProgressToClinicalEvent() override = default;

  [[nodiscard]] const std::string name() const override { return "ProgressToClinicalEvent"; }

  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) {
    clinical_caused_parasite_ = value;
  }

  static bool should_receive_treatment(Person* person);

  static void handle_no_treatment(Person* person);

  static std::pair<Therapy*, bool> determine_therapy(Person* person, bool is_recurrence = false);

  void transition_to_clinical_state(Person* person);

  void apply_therapy(Person* person, Therapy* therapy, bool is_public_sector = true);

private:
  bool is_recurrence_ = false;

public:
  void set_is_recurrence(bool v) { is_recurrence_ = v; }
  bool is_recurrence() const { return is_recurrence_; }


private:
  ClonalParasitePopulation* clinical_caused_parasite_{nullptr};
  void do_execute() override;
};

#endif /* PROGRESSTOCLINICALEVENT_H */
