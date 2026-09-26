#ifndef ENDCLINICALEVENT_H
#define ENDCLINICALEVENT_H

// #include "Core/ObjectPool.h"
#include <cstddef>

#include <cstdint>

#include "Event.h"
#include "Population/ClonalParasitePopulation.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class EndClinicalEvent : public PersonEvent {
  //  OBJECTPOOL(EndClinicalEvent)
public:
  // disallow copy, assign and move
  EndClinicalEvent(const EndClinicalEvent &) = delete;
  EndClinicalEvent &operator=(const EndClinicalEvent &) = delete;
  EndClinicalEvent(EndClinicalEvent &&) = delete;
  EndClinicalEvent &operator=(EndClinicalEvent &&) = delete;
  explicit EndClinicalEvent(Person* person) : PersonEvent(person) {}
  ~EndClinicalEvent() override = default;

  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  [[nodiscard]] std::uint64_t clinical_caused_parasite_uid() const { return clinical_caused_parasite_uid_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) {
    clinical_caused_parasite_ = value;
    clinical_caused_parasite_uid_ = (value != nullptr) ? value->uid() : 0;
  }

  static constexpr std::string_view EVENT_NAME{"EndClinicalEvent"};
  [[nodiscard]] std::string_view name() const noexcept override { return EVENT_NAME; }

private:
  ClonalParasitePopulation* clinical_caused_parasite_{nullptr};
  std::uint64_t clinical_caused_parasite_uid_{0};
  void do_execute() override;
};

#endif /* ENDCLINICALEVENT_H */
