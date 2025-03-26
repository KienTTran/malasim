#ifndef RECEIVETHERAPYEVENT_H
#define    RECEIVETHERAPYEVENT_H

#include "Event.h"
#include "Population/ClonalParasitePopulation.h"

class Scheduler;

class Person;

class Therapy;

class ClonalParasitePopulation;

class ReceiveTherapyEvent : public Event {
public:
  //disallow copy and assign and move
  ReceiveTherapyEvent(const ReceiveTherapyEvent &) = delete;
  void operator=(const ReceiveTherapyEvent &) = delete;
  ReceiveTherapyEvent(ReceiveTherapyEvent &&) = delete;
  ReceiveTherapyEvent operator=(ReceiveTherapyEvent &&) = delete;

private:
  Therapy *received_therapy_ = nullptr;
    ClonalParasitePopulation *clinical_caused_parasite_ = nullptr;
public:
  Therapy *received_therapy() { return received_therapy_; }
  void set_received_therapy(Therapy *value) { received_therapy_ = value; }
  ClonalParasitePopulation *clinical_caused_parasite() { return clinical_caused_parasite_; }
  void set_clinical_caused_parasite(ClonalParasitePopulation *value) { clinical_caused_parasite_ = value; }

public:
  bool is_part_of_MAC_therapy { false };

public:
  ReceiveTherapyEvent();

  //    ReceiveTherapyEvent(const ReceiveTherapyEvent& orig);
  virtual ~ReceiveTherapyEvent();

  static void schedule_event(Scheduler *scheduler, Person *p, Therapy *therapy, const int &time,
                             ClonalParasitePopulation *clinical_caused_parasite, bool is_part_of_MAC_therapy = false);

  std::string name() override {
    return "ReceiveTherapyEvent";
  }

 private:
  void execute() override;
};

#endif    /* RECEIVETHERAPYEVENT_H */
