#ifndef RECEIVESMCDRUGEVENT_H
#define RECEIVESMCDRUGEVENT_H

#include <string>

//#include "Core/PropertyMacro.h"
#include "Event.h"

class Scheduler;

class Person;

class Therapy;

class ReceiveSMCTherapyEvent : public PersonEvent {
public:
  //disable copy and move
  ReceiveSMCTherapyEvent(const ReceiveSMCTherapyEvent&) = delete;
  ReceiveSMCTherapyEvent(ReceiveSMCTherapyEvent&&) = delete;
//  DELETE_COPY_AND_MOVE(ReceiveSMCTherapyEvent)

//  POINTER_PROPERTY(Therapy, received_therapy)
private:
  Therapy* received_therapy_{nullptr};
public:
  Therapy* received_therapy() { return received_therapy_; }
  void set_received_therapy(Therapy* value) { received_therapy_ = value; }

public:
  ReceiveSMCTherapyEvent(Person* person) : PersonEvent(person), received_therapy_(nullptr) {}

  //    ReceiveMDADrugEvent(const ReceiveMDADrugEvent& orig);
  virtual ~ReceiveSMCTherapyEvent() = default;

  const std::string name() const override { return "ReceiveSMCTherapyEvent"; }

private:
  void do_execute() override;
};

#endif /* RECEIVESMCDRUGEVENT_H */
