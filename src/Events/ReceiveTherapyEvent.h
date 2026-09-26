#ifndef RECEIVETHERAPYEVENT_H
#define RECEIVETHERAPYEVENT_H

#include <cstdint>

#include "Event.h"
#include "Population/ClonalParasitePopulation.h"

class Scheduler;

class Person;

class Therapy;

class ClonalParasitePopulation;

class ReceiveTherapyEvent : public PersonEvent {
public:
  // disallow copy and assign and move
  ReceiveTherapyEvent(const ReceiveTherapyEvent &) = delete;
  ReceiveTherapyEvent &operator=(const ReceiveTherapyEvent &) = delete;
  ReceiveTherapyEvent(ReceiveTherapyEvent &&) = delete;
  ReceiveTherapyEvent &operator=(ReceiveTherapyEvent &&) = delete;
  explicit ReceiveTherapyEvent(Person* person) : PersonEvent(person) {}
  ~ReceiveTherapyEvent() override = default;

  static constexpr std::string_view EVENT_NAME{"ReceiveTherapyEvent"};
  [[nodiscard]] std::string_view name() const noexcept override { return EVENT_NAME; }

  Therapy* received_therapy() { return received_therapy_; }
  void set_received_therapy(Therapy* value) { received_therapy_ = value; }

  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  [[nodiscard]] std::uint64_t clinical_caused_parasite_uid() const { return clinical_caused_parasite_uid_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) {
    clinical_caused_parasite_ = value;
    clinical_caused_parasite_uid_ = (value != nullptr) ? value->uid() : 0;
  }

  [[nodiscard]] bool is_part_of_mac_therapy() const { return is_part_of_mac_therapy_; }
  void set_is_part_of_mac_therapy(bool value) { is_part_of_mac_therapy_ = value; }

private:
  bool is_part_of_mac_therapy_{false};
  Therapy* received_therapy_{nullptr};
  ClonalParasitePopulation* clinical_caused_parasite_{nullptr};
  std::uint64_t clinical_caused_parasite_uid_{0};
  void do_execute() override;
};

#endif /* RECEIVETHERAPYEVENT_H */
