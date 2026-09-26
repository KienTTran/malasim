#ifndef PROGRESSTOCLINICALEVENT_H
#define PROGRESSTOCLINICALEVENT_H

#include <cstdint>

#include "Event.h"
#include "Population/ClonalParasitePopulation.h"
#include "Treatment/Strategies/TreatmentSelection.h"
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
  ProgressToClinicalEvent(const ProgressToClinicalEvent &) = delete;
  ProgressToClinicalEvent &operator=(const ProgressToClinicalEvent &) = delete;

  // Disallow move
  ProgressToClinicalEvent(ProgressToClinicalEvent &&) = delete;
  ProgressToClinicalEvent &operator=(ProgressToClinicalEvent &&) = delete;

  explicit ProgressToClinicalEvent(Person* person) : PersonEvent(person) {}

  ~ProgressToClinicalEvent() override = default;
  static constexpr std::string_view EVENT_NAME{"ProgressToClinicalEvent"};
  [[nodiscard]] std::string_view name() const noexcept override { return EVENT_NAME; }

  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  [[nodiscard]] std::uint64_t clinical_caused_parasite_uid() const { return clinical_caused_parasite_uid_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) {
    clinical_caused_parasite_ = value;
    clinical_caused_parasite_uid_ = (value != nullptr) ? value->uid() : 0;
  }

  [[nodiscard]] bool is_recurrence() const { return is_recurrence_; }
  void set_is_recurrence(const bool value) { is_recurrence_ = value; }

  static bool should_receive_treatment(Person* person);

  static void handle_no_treatment(Person* person);

  static TreatmentSelection determine_therapy(Person* person, bool is_recurrence = false);

  void transition_to_clinical_state(Person* person);

  void apply_therapy(Person* person, Therapy* therapy, bool is_public_sector = true);

private:
  ClonalParasitePopulation* clinical_caused_parasite_{nullptr};
  std::uint64_t clinical_caused_parasite_uid_{0};
  bool is_recurrence_{false};
  void do_execute() override;
};

#endif /* PROGRESSTOCLINICALEVENT_H */
