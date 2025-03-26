#ifndef SINGLEROUNDMDAEVENT_H
#define SINGLEROUNDMDAEVENT_H

#include "Events/Event.h"
#include <vector>

class SingleRoundMDAEvent : public Event {
public:
  //disallow copy, assign and move
  SingleRoundMDAEvent(const SingleRoundMDAEvent &) = delete;
  void operator=(const SingleRoundMDAEvent &) = delete;
  SingleRoundMDAEvent(SingleRoundMDAEvent &&) = delete;
  SingleRoundMDAEvent operator=(SingleRoundMDAEvent &&) = delete;

 public:
  std::vector<double> fraction_population_targeted;
  int days_to_complete_all_treatments{14};

  explicit SingleRoundMDAEvent(const int &execute_at);

  virtual ~SingleRoundMDAEvent() = default;

  std::string name() override {
    return "SingleRoundMDAEvent";
  }

 private:
  void execute() override;
};

#endif // SINGLEROUNDMDAEVENT_H
