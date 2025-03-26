#ifndef MATUREGAMETOCYTEEVENT_H
#define MATUREGAMETOCYTEEVENT_H

// #include "Core/ObjectPool.h"
// #include "Core/PropertyMacro.h"
#include "Event.h"

class ClonalParasitePopulation;

class Scheduler;

class Person;

class MatureGametocyteEvent : public Event {
public:
  //disallow copy, assign and move
  MatureGametocyteEvent(const MatureGametocyteEvent&) = delete;
  void operator=(const MatureGametocyteEvent&) = delete;
  MatureGametocyteEvent(MatureGametocyteEvent&&) = delete;
  MatureGametocyteEvent operator=(MatureGametocyteEvent&&) = delete;

  // OBJECTPOOL(MatureGametocyteEvent)
private:
  ClonalParasitePopulation* blood_parasite_;
public:
  ClonalParasitePopulation* blood_parasite() { return blood_parasite_; }
  void set_blood_parasite(ClonalParasitePopulation* value) { blood_parasite_ = value; }

public:
  MatureGametocyteEvent();

  //    MatureGametocyteEvent(const MatureGametocyteEvent& orig);
  virtual ~MatureGametocyteEvent();

  static void schedule_event(Scheduler* scheduler, Person* p,
                             ClonalParasitePopulation* blood_parasite,
                             const int &time);

  std::string name() override { return "MatureGametocyteEvent"; }

private:
  void execute() override;
};

#endif /* MATUREGAMETOCYTEEVENT_H */
