//
// Created by Kien Tran on 3/17/25.
//

#ifndef SEASONALRAINFALL_H
#define SEASONALRAINFALL_H


#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <fmt/format.h>
#include "SeasonalInfo.h"
#include "Utils/Helpers/TimeHelpers.h"

class SeasonalRainfall : public ISeasonalInfo {
public:
  ~SeasonalRainfall() override = default;
  SeasonalRainfall() = default;

  void build();
  double get_seasonal_factor(const date::sys_days &today, const int &location) override;
  void read(const std::string &filename);

  [[nodiscard]] std::string get_filename() const;
  void set_filename(const std::string &value);
  [[nodiscard]] int get_period() const;
  void set_period(int value);

private:
  std::string filename_;
  std::vector<double> adjustments_;
  int period_ = 12;
};


#endif //SEASONALRAINFALL_H
