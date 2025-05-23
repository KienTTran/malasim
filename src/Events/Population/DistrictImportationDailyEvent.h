#ifndef DISTRICTIMPORTATIONDAILYEVENT_H
#define DISTRICTIMPORTATIONDAILYEVENT_H

#include <vector>
#include <tuple>
#include "Events/Event.h"

class Scheduler;
class DistrictImportationDailyEvent : public WorldEvent {
private:
  int district_;
  double daily_rate_;
  std::vector<std::tuple<int,int,char>> alleles_;
public:
  explicit DistrictImportationDailyEvent(int district = -1,
                                         double dailyRate = -1,
                                         int startDay = -1,
                                         const std::vector<std::tuple<int,int,char>> &alleles = {});

  DistrictImportationDailyEvent(const DistrictImportationDailyEvent &) = delete;
  DistrictImportationDailyEvent(DistrictImportationDailyEvent &&) = delete;
  DistrictImportationDailyEvent &operator=(
      const DistrictImportationDailyEvent &) = delete;
  DistrictImportationDailyEvent &operator=(DistrictImportationDailyEvent &&) =
      delete;
  ~DistrictImportationDailyEvent() override = default;

  inline static const std::string EventName =
      "district_importation_daily_event";

  const std::string name() const override { return "DistrictImportationDailyEvent"; }

private:
  void do_execute() override;
};

#endif  // DISTRICTIMPORTATIONDAILYEVENT_H
