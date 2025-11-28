/* 
 * File:   EndClinicalByNoTreatmentEvent.h
 * Author: Merlin
 *
 * Created on July 31, 2013, 12:28 PM
 */

#ifndef ENDCLINICALBYNOTREATMENTEVENT_H
#define    ENDCLINICALBYNOTREATMENTEVENT_H

#include "Event.h"
// #include "Core/ObjectPool.h"
// #include "Core/PropertyMacro.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class EndClinicalByNoTreatmentEvent : public PersonEvent {
public:
  // disallow copy, assign and move
  EndClinicalByNoTreatmentEvent(const EndClinicalByNoTreatmentEvent &) = delete;
  EndClinicalByNoTreatmentEvent& operator=(const EndClinicalByNoTreatmentEvent &) = delete;
  EndClinicalByNoTreatmentEvent(EndClinicalByNoTreatmentEvent &&) = delete;
  EndClinicalByNoTreatmentEvent& operator=(EndClinicalByNoTreatmentEvent &&) = delete;
  explicit EndClinicalByNoTreatmentEvent(Person* person) : PersonEvent(person) {}
  ~EndClinicalByNoTreatmentEvent() override = default;

  [[nodiscard]] const std::string name() const override {
    return "EndClinicalByNoTreatmentEvent";
  }

  void set_clinical_caused_parasite(ClonalParasitePopulation* value) {
    clinical_caused_parasite_ = value;
  }

private:
  ClonalParasitePopulation* clinical_caused_parasite_{nullptr};
  void do_execute() override;
};

#endif    /* ENDCLINICALBYNOTREATMENTEVENT_H */
