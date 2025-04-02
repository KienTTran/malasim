#ifndef ENDCLINICALEVENT_H
#define ENDCLINICALEVENT_H

//#include "Core/ObjectPool.h"
#include "Event.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class EndClinicalEvent : public Event {
public:
  //disallow copy, assign and move
  EndClinicalEvent(const EndClinicalEvent&) = delete;
  void operator=(const EndClinicalEvent&) = delete;
  EndClinicalEvent(EndClinicalEvent&&) = delete;
  void operator=(EndClinicalEvent&&) = delete;

//  OBJECTPOOL(EndClinicalEvent)
private:
    ClonalParasitePopulation* clinical_caused_parasite_;
public:
    ClonalParasitePopulation* clinical_caused_parasite() { return clinical_caused_parasite_; }
    void set_clinical_caused_parasite(ClonalParasitePopulation* value) { clinical_caused_parasite_ = value; }

public:
  EndClinicalEvent();

  //    EndClinicalEvent(const EndClinicalEvent& orig);
  virtual ~EndClinicalEvent();

  const std::string name() const override { return "EndClinicalEvent"; }

private:
  void do_execute() override;
};

#endif /* ENDCLINICALEVENT_H */
