/*
 * SeasonalInfo.h
 *
 * Define the base interface for seasonal information methods
 */
#ifndef SEASONALINFO_H
#define SEASONALINFO_H

#include <date/date.h>

class ISeasonalInfo {
public:
    //disallow copy and assign and move
    ISeasonalInfo(const ISeasonalInfo&) = delete;
    void operator=(const ISeasonalInfo&) = delete;
    ISeasonalInfo(ISeasonalInfo&&) = delete;
    ISeasonalInfo& operator=(ISeasonalInfo&&) = delete;
  // Return the seasonal factor for the given day and location, based upon the
  // loaded configuration.
  virtual double get_seasonal_factor(const date::sys_days &today,
                                   const int &location) {
    throw std::runtime_error("Runtime call to virtual function");
  }
  virtual ~ISeasonalInfo() = default;
};

#endif
