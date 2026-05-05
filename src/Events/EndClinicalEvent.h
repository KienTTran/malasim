#ifndef ENDCLINICALEVENT_H
#define ENDCLINICALEVENT_H

// #include "Core/ObjectPool.h"
#include <cstddef>

#include "Event.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class EndClinicalEvent : public PersonEvent {
  //  OBJECTPOOL(EndClinicalEvent)
public:
  // disallow copy, assign and move
  EndClinicalEvent(const EndClinicalEvent &) = delete;
  EndClinicalEvent& operator=(const EndClinicalEvent &) = delete;
  EndClinicalEvent(EndClinicalEvent &&) = delete;
  EndClinicalEvent& operator=(EndClinicalEvent &&) = delete;
  explicit EndClinicalEvent(Person* person) : PersonEvent(person) {}
  ~EndClinicalEvent() override = default;

  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) {
    clinical_caused_parasite_ = value;
  }

  [[nodiscard]] const std::string name() const override { return "EndClinicalEvent"; }

private:
  ClonalParasitePopulation* clinical_caused_parasite_{nullptr};
  void do_execute() override;

private:
  bool is_recurrence_ = false;

public:
  void set_is_recurrence(bool value) { is_recurrence_ = value; }
  bool is_recurrence() const { return is_recurrence_; }
};

#endif /* ENDCLINICALEVENT_H */
