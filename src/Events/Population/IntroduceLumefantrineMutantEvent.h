#pragma once

#include <string>

// #include "Core/ObjectPool.h"
#include "Events/Event.h"

class IntroduceLumefantrineMutantEvent : public Event {
  //disallow copy, assign and move
  IntroduceLumefantrineMutantEvent(const IntroduceLumefantrineMutantEvent &) = delete;
  void operator=(const IntroduceLumefantrineMutantEvent &) = delete;
  IntroduceLumefantrineMutantEvent(IntroduceLumefantrineMutantEvent &&) = delete;
  IntroduceLumefantrineMutantEvent operator=(IntroduceLumefantrineMutantEvent &&) = delete;

  // OBJECTPOOL(IntroduceLumefantrineMutantEvent)

private:
  int location_;
  double fraction_;
  std::vector<std::tuple<int,int,char>> alleles_;

public:
  explicit IntroduceLumefantrineMutantEvent(const int &location = -1,
                                              const int &execute_at = -1,
                                            const double &fraction = 0,
                                            const std::vector<std::tuple<int,int,char>> &allele_map = {});

  //    ImportationEvent(const ImportationEvent& orig);
  ~IntroduceLumefantrineMutantEvent() override;

  std::string name() override { return "IntroduceLumefantrineMutantEvent"; }

private:
  void execute() override;
};
