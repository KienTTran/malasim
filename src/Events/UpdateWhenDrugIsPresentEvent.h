#ifndef UPDATEWHENDRUGISPRESENTEVENT_H
#define UPDATEWHENDRUGISPRESENTEVENT_H

#include <cstdint>

#include "Event.h"
#include "Population/ClonalParasitePopulation.h"
// #include "Core/ObjectPool.h"
// #include "Core/PropertyMacro.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class UpdateWhenDrugIsPresentEvent : public PersonEvent {
  // OBJECTPOOL(UpdateWhenDrugIsPresentEvent)
public:
  // Disallow copy
  UpdateWhenDrugIsPresentEvent(const UpdateWhenDrugIsPresentEvent &) = delete;
  UpdateWhenDrugIsPresentEvent &operator=(const UpdateWhenDrugIsPresentEvent &) = delete;

  // Disallow move
  UpdateWhenDrugIsPresentEvent(UpdateWhenDrugIsPresentEvent &&) = delete;
  UpdateWhenDrugIsPresentEvent &operator=(UpdateWhenDrugIsPresentEvent &&) = delete;

  explicit UpdateWhenDrugIsPresentEvent(Person* person) : PersonEvent(person) {}

  ~UpdateWhenDrugIsPresentEvent() override = default;

  static constexpr std::string_view EVENT_NAME{"UpdateByHavingDrugEvent"};
  [[nodiscard]] std::string_view name() const noexcept override { return EVENT_NAME; }

  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  [[nodiscard]] std::uint64_t clinical_caused_parasite_uid() const { return clinical_caused_parasite_uid_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) {
    clinical_caused_parasite_ = value;
    clinical_caused_parasite_uid_ = (value != nullptr) ? value->uid() : 0;
  }

private:
  ClonalParasitePopulation* clinical_caused_parasite_{nullptr};
  std::uint64_t clinical_caused_parasite_uid_{0};

  void do_execute() override;
};

#endif /* UPDATEWHENDRUGISPRESENTEVENT_H */
