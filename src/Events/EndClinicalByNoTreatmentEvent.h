#ifndef ENDCLINICALBYNOTREATMENTEVENT_H
#define ENDCLINICALBYNOTREATMENTEVENT_H

//#include "Core/ObjectPool.h"
#include "Event.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class EndClinicalByNoTreatmentEvent : public Event {
public:
  //disallow copy, assign and move
  EndClinicalByNoTreatmentEvent(const EndClinicalByNoTreatmentEvent&) = delete;
  void operator=(const EndClinicalByNoTreatmentEvent&) = delete;
  EndClinicalByNoTreatmentEvent(EndClinicalByNoTreatmentEvent&&) = delete;
  EndClinicalByNoTreatmentEvent operator=(EndClinicalByNoTreatmentEvent&&) = delete;

//  OBJECTPOOL(EndClinicalByNoTreatmentEvent)

//  POINTER_PROPERTY(ClonalParasitePopulation, clinical_caused_parasite)
private:
  ClonalParasitePopulation* clinical_caused_parasite_;
public:
  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) { clinical_caused_parasite_ = value; }

public:
  EndClinicalByNoTreatmentEvent();

  //    EndClinicalByNoTreatmentEvent(const EndClinicalByNoTreatmentEvent&
  //    orig);
  virtual ~EndClinicalByNoTreatmentEvent();

  static void schedule_event(Scheduler* scheduler, Person* p,
                             ClonalParasitePopulation* clinical_caused_parasite,
                             const int &time);

  std::string name() override { return "EndClinicalByNoTreatmentEvent"; }

private:
  void execute() override;
};

#endif /* ENDCLINICALBYNOTREATMENTEVENT_H */
