#ifndef PROGRESSTOCLINICALEVENT_H
#define    PROGRESSTOCLINICALEVENT_H

#include "Event.h"
// #include "Core/ObjectPool.h"
#include <string>

class Person;

class Scheduler;

class ClonalParasitePopulation;

class ProgressToClinicalEvent : public Event {
public:
  //disallow copy, assign and move
  ProgressToClinicalEvent(const ProgressToClinicalEvent&) = delete;
  void operator=(const ProgressToClinicalEvent&) = delete;
  ProgressToClinicalEvent(ProgressToClinicalEvent&&) = delete;
  ProgressToClinicalEvent operator=(ProgressToClinicalEvent&&) = delete;

 // OBJECTPOOL(ProgressToClinicalEvent)
private:
  ClonalParasitePopulation* clinical_caused_parasite_;
public:
  ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation* value) { clinical_caused_parasite_ = value; }

 public:
  ProgressToClinicalEvent();

  virtual ~ProgressToClinicalEvent();

  static void schedule_event(Scheduler *scheduler, Person *p, ClonalParasitePopulation *clinical_caused_parasite,
                             const int &time);

  static void receive_no_treatment_routine(Person *p);

  std::string name() override {
    return "ProgressToClinicalEvent";
  }

 private:
  void execute() override;
};

#endif    /* PROGRESSTOCLINICALEVENT_H */
