/*
 * TestTreatmentFailureEvent.cpp
 *
 * Defines the event that tests to see if the treatment given to a patient
 * failed or not.
 */
#ifndef TESTTREATMENTFAILUREEVENT_H
#define TESTTREATMENTFAILUREEVENT_H

//#include "Core/ObjectPool.h"
//#include "Core/PropertyMacro.h"
#include "Event.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class TestTreatmentFailureEvent : public Event {
public:
  //disallow copy, assign and move
  TestTreatmentFailureEvent(const TestTreatmentFailureEvent&) = delete;
  void operator=(const TestTreatmentFailureEvent&) = delete;
  TestTreatmentFailureEvent(TestTreatmentFailureEvent&&) = delete;
  TestTreatmentFailureEvent operator=(TestTreatmentFailureEvent&&) = delete;

//  OBJECTPOOL(TestTreatmentFailureEvent)
private:
    int therapyId_;
    ClonalParasitePopulation* clinical_caused_parasite_;
public:
    ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
    void set_clinical_caused_parasite(ClonalParasitePopulation* value) { clinical_caused_parasite_ = value; }
    int therapyId() { return therapyId_; }
    void set_therapyId(int value) { therapyId_ = value; }

public:
  TestTreatmentFailureEvent();
  ~TestTreatmentFailureEvent() override = default;

  static void schedule_event(Scheduler* scheduler, Person* p,
                             ClonalParasitePopulation* clinical_caused_parasite,
                             const int &time, const int &t_id = 0);

  std::string name() override { return "TestTreatmentFailureEvent"; }

private:
  void execute() override;
};

#endif
