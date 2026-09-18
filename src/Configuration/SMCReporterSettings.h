#ifndef SMCReporterSETTINGS_H
#define SMCReporterSETTINGS_H
#include "IConfigData.h"
#include <date/date.h>
#include <yaml-cpp/yaml.h>


class SMCReporterSettings : public IConfigData{

public:

  [[nodiscard]] date::year_month_day get_smc_reporting_start_day() const { return smc_reporting_start_day;}
  void set_smc_reporting_start_day(const date::year_month_day value) { smc_reporting_start_day = value; }
  [[nodiscard]] date::year_month_day get_smc_reporting_end_day() const { return smc_reporting_end_day;}
  void set_smc_reporting_end_day(const date::year_month_day value) { smc_reporting_end_day = value; }
  [[nodiscard]] int get_smc_reporting_number_of_people_tracked() const { return smc_reporting_number_of_people_tracked;}
  void set_smc_reporting_number_of_people_tracked(const int value) { smc_reporting_number_of_people_tracked = value; }
  [[nodiscard]] bool get_smc_reporting_track_per_district() const { return smc_reporting_track_per_district;}
  void set_smc_reporting_track_per_district(const bool value) { smc_reporting_track_per_district = value; }
  [[nodiscard]] bool get_smc_refresh_samples_each_interval() const { return smc_refresh_samples_each_interval;}
  void set_smc_refresh_samples_each_interval(const bool value) { smc_refresh_samples_each_interval = value; }
  [[nodiscard]] int get_smc_reporting_interval() const { return smc_reporting_interval;}
  void set_smc_reporting_interval(const int value) { smc_reporting_interval = value; }

private:
  date::year_month_day smc_reporting_start_day = date::year_month_day(date::year(2000), date::month(1), date::day(1));
  date::year_month_day smc_reporting_end_day = date::year_month_day(date::year(2000), date::month(12), date::day(31));
  int smc_reporting_number_of_people_tracked = 100;
  int smc_reporting_interval = 1; // Default to 1 day interval
  bool smc_reporting_track_per_district = false;
  bool smc_refresh_samples_each_interval = true;

public:
  void process_config() override {}

};
// Specialization of convert for the SimulationTimeframe class
template <>
struct YAML::convert<SMCReporterSettings> {
  static Node encode(const SMCReporterSettings &rhs) {
    Node node;
    node["smc_reporting_start_day"] = rhs.get_smc_reporting_start_day();
    node["smc_reporting_end_day"] = rhs.get_smc_reporting_end_day();
    node["smc_reporting_number_of_people_tracked"] = rhs.get_smc_reporting_number_of_people_tracked();
    node["smc_reporting_track_per_district"] = rhs.get_smc_reporting_track_per_district();
    node["smc_refresh_samples_each_interval"] = rhs.get_smc_refresh_samples_each_interval();
    node["smc_reporting_interval"] = rhs.get_smc_reporting_interval();
    return node;
  }

  static bool decode(const Node &node, SMCReporterSettings &rhs) {


      if (!node["smc_reporting_start_day"]) {
        throw std::runtime_error("Missing 'smc_reporting_start_day' field in SMCReporterSettings.");
      }
      if (!node["smc_reporting_end_day"]) {
        throw std::runtime_error("Missing 'smc_reporting_end_day' field in SMCReporterSettings.");
      }
      if (!node["smc_reporting_number_of_people_tracked"]) {
        throw std::runtime_error("Missing 'smc_reporting_number_of_people_tracked' field in SMCReporterSettings.");
      }
      if (!node["smc_reporting_track_per_district"]) {
        throw std::runtime_error("Missing 'smc_reporting_track_per_district' field in SMCReporterSettings.");
      }
      if (!node["smc_refresh_samples_each_interval"]) {
        throw std::runtime_error("Missing 'smc_refresh_samples_each_interval' field in SMCReporterSettings.");
      }
      if (!node["smc_reporting_interval"]) {
        throw std::runtime_error("Missing 'smc_reporting_interval' field in SMCReporterSettings.");
      }
      rhs.set_smc_reporting_start_day(node["smc_reporting_start_day"].as<date::year_month_day>());
      rhs.set_smc_reporting_end_day(node["smc_reporting_end_day"].as<date::year_month_day>());
      rhs.set_smc_reporting_number_of_people_tracked(node["smc_reporting_number_of_people_tracked"].as<int>());
      rhs.set_smc_reporting_track_per_district(node["smc_reporting_track_per_district"].as<bool>());
      rhs.set_smc_refresh_samples_each_interval(node["smc_refresh_samples_each_interval"].as<bool>());
      rhs.set_smc_reporting_interval(node["smc_reporting_interval"].as<int>());

      return true;
  }
};
#endif //SMCREPORTERSETTINGS_H
