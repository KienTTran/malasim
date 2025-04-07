//
// Created by Kien Tran on 3/17/25.
//

#ifndef SEASONALPATTERN_H
#define SEASONALPATTERN_H

#include <string>
#include <vector>

#include "SeasonalInfo.h"

class SpatialData;

class SeasonalPattern : public ISeasonalInfo {
public:
  void build(SpatialData* spatial_data);
  void read(const std::string &filename);
  double get_seasonal_factor(const date::sys_days &today, const int &location) override;

  [[nodiscard]] int get_period() const { return period; }

  void set_period(int period) { this->period = period; }

  [[nodiscard]] bool get_is_monthly() const { return is_monthly; }

  void set_is_monthly(bool is_monthly) { this->is_monthly = is_monthly; }

  [[nodiscard]] int get_min_admin_unit_id() const { return min_admin_unit_id; }

  void set_min_admin_unit_id(int min_admin_unit_id) { this->min_admin_unit_id = min_admin_unit_id; }

  [[nodiscard]] int get_max_admin_unit_id() const { return max_admin_unit_id; }

  void set_max_admin_unit_id(int max_admin_unit_id) { this->max_admin_unit_id = max_admin_unit_id; }

  [[nodiscard]] std::string get_filename() const { return filename; }

  void set_filename(const std::string &filename) { this->filename = filename; }

  [[nodiscard]] std::string get_admin_level() const { return admin_level; }
  void set_admin_level(const std::string &admin_level) { this->admin_level = admin_level; }

  // Make this virtual so we can override it in tests
  [[nodiscard]] virtual int get_admin_unit_for_location(int location) const;

protected:
  // Vector of vectors: [admin_unit][day/month]
  std::vector<std::vector<double>> admin_unit_adjustments;
  int period{12};         // Either 365 for daily or 12 for monthly
  bool is_monthly{true};  // Flag to indicate if we're using monthly data

  int min_admin_unit_id{-1};
  int max_admin_unit_id{-1};
  std::string admin_level;
  std::string filename;
  int admin_level_id{-1};
};

#endif  // SEASONALPATTERN_H
