#ifndef PCMS_INTRODUCE580YPARASITEEVENT_H
#define PCMS_INTRODUCE580YPARASITEEVENT_H

//#include "Core/ObjectPool.h"
#include "Events/Event.h"
#include <string>
#include <vector>
#include <tuple>

class Introduce580YMutantEvent : public WorldEvent {
public:
    // Disallow copy
    Introduce580YMutantEvent(const Introduce580YMutantEvent&) = delete;
    Introduce580YMutantEvent& operator=(const Introduce580YMutantEvent&) = delete;

    // Disallow move
    Introduce580YMutantEvent(Introduce580YMutantEvent&&) = delete;
    Introduce580YMutantEvent& operator=(Introduce580YMutantEvent&&) = delete;

//    OBJECTPOOL(Introduce580YMutantEvent)
private:
  int location_;
  double fraction_;
  std::vector<std::tuple<int,int,char>> alleles_;

public:
  explicit Introduce580YMutantEvent(const int& location = -1, const int& execute_at = -1,
                                    const double& fraction = 0,
                                    const std::vector<std::tuple<int,int,char>> &allele_map = {});

  //    ImportationEvent(const ImportationEvent& orig);
  ~Introduce580YMutantEvent() override;

  [[nodiscard]] const std::string name() const override {
    return "580YImportationEvent";
  }

private:
  void do_execute() override;

};

#endif //PCMS_INTRODUCE580YPARASITEEVENT_H
