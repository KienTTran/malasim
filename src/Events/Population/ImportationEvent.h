#ifndef IMPORTATIONEVENT_H
#define IMPORTATIONEVENT_H

#include <string>

//#include "Core/ObjectPool.h"
#include "Events/Event.h"

class ImportationEvent : public Event {
public:
  //disallow copy, assign and move
  ImportationEvent(const ImportationEvent&) = delete;
  void operator=(const ImportationEvent&) = delete;
  ImportationEvent(ImportationEvent&&) = delete;
  ImportationEvent operator=(ImportationEvent&&) = delete;

  // OBJECTPOOL(ImportationEvent)
private:
  int location_;
    int genotype_id_;
    int number_of_cases_;
public:
  int get_location() const { return location_; }
  int get_genotype_id() const { return genotype_id_; }
  int get_number_of_cases() const { return number_of_cases_; }

public:
  ImportationEvent(const int &location = -1, const int &execute_at = -1,
                   const int &genotype_id = -1,
                   const int &number_of_cases = -1);

  //    ImportationEvent(const ImportationEvent& orig);
  virtual ~ImportationEvent();

  static void schedule_event(Scheduler* scheduler, const int &location,
                             const int &execute_at, const int &genotype_id,
                             const int &number_of_cases);

  std::string name() override { return "ImportationEvent"; }

private:
  void execute() override;
};

#endif /* IMPORTATIONEVENT_H */
