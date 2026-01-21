#ifndef SIMULATION_TIMEFRAME_H
#define SIMULATION_TIMEFRAME_H

#include <Utils/Helpers/TimeHelpers.h>
#include <yaml-cpp/yaml.h>
#include <date/date.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

#include "IConfigData.h"
#include "Utils/YamlFile.h"

// Class to hold the simulation timeframe data
class SimulationTimeframe : public IConfigData{
public:
  // Getters and Setters for starting_date
  [[nodiscard]] const date::year_month_day &get_starting_date() const {
    return starting_date_;
  }

  void set_starting_date(const date::year_month_day &value) {
    starting_date_ = value;
  }

  // Getters and Setters for start_of_comparison_period
  [[nodiscard]] const date::year_month_day &get_start_of_comparison_period_date() const {
    return start_of_comparison_period_date_;
  }

  void set_start_of_comparison_period_date(const date::year_month_day &value) {
    if (value < starting_date_) {
      throw std::invalid_argument("start_of_comparison_period cannot be before starting_date");
    }
    start_of_comparison_period_date_ = value;
  }

  [[nodiscard]] int get_start_of_comparison_period() const {
    return start_of_comparison_period_;
  }

  void set_start_of_comparison_period(const int value) {
    start_of_comparison_period_ = value;
  }

  // Getters and Setters for ending_date
  [[nodiscard]] const date::year_month_day &get_ending_date() const {
    return ending_date_;
  }

  void set_ending_date(const date::year_month_day &value) {
    if (value < start_of_comparison_period_date_) {
      throw std::invalid_argument("ending_date cannot be before start_of_comparison_period");
    }
    ending_date_ = value;
  }

  // Getters and Setters for start_collect_data_day
  [[nodiscard]] int get_start_collect_data_day() const {
    return start_collect_data_day_;
  }

  void set_start_collect_data_day(const int value) {
    if (value < 0) {
      throw std::invalid_argument("start_collect_data_day must be non-negative");
    }
    start_collect_data_day_ = value;
  }

  void set_total_time(const int total_time) {
    total_time_ = total_time;
  }

  [[nodiscard]] int get_total_time() const {
    return total_time_;
  }

  void process_config() override {
    spdlog::info("Processing SimulationTimeframe");
    total_time_ = TimeHelpers::days_between(starting_date_, ending_date_);
    start_of_comparison_period_ = TimeHelpers::days_between(starting_date_, start_of_comparison_period_date_);
  }

private:
  date::year_month_day starting_date_ = date::year_month_day(date::year(2000), date::month(1), date::day(1));
  date::year_month_day start_of_comparison_period_date_ = date::year_month_day(date::year(2000), date::month(1), date::day(1));
  int start_of_comparison_period_ = 0;
  date::year_month_day ending_date_ = date::year_month_day(date::year(2000), date::month(1), date::day(1));
  int start_collect_data_day_ = 0;
  int total_time_ = 0;
};

// Specialization of convert for the SimulationTimeframe class
template <>
struct YAML::convert<SimulationTimeframe> {
  static Node encode(const SimulationTimeframe &rhs) {
    Node node;
    node["starting_date"] = format("%Y/%m/%d", rhs.get_starting_date());
    node["start_of_comparison_period"] = format("%Y/%m/%d", rhs.get_start_of_comparison_period_date());
    node["ending_date"] = format("%Y/%m/%d", rhs.get_ending_date());
    node["start_collect_data_day"] = rhs.get_start_collect_data_day();
    return node;
  }

  static bool decode(const Node &node, SimulationTimeframe &rhs) {
    if (!node["starting_date"]) {
      throw std::runtime_error("Missing 'starting_date' field.");
    }
    if (!node["start_of_comparison_period"]) {
      throw std::runtime_error("Missing 'start_of_comparison_period' field.");
    }
    if (!node["ending_date"]) {
      throw std::runtime_error("Missing 'ending_date' field.");
    }
    if (!node["start_collect_data_day"]) {
      throw std::runtime_error("Missing 'start_collect_data_day' field.");
    }

    // Parsing and assigning the values
    rhs.set_starting_date(node["starting_date"].as<date::year_month_day>());
    rhs.set_start_of_comparison_period_date(node["start_of_comparison_period"].as<date::year_month_day>());
    rhs.set_ending_date(node["ending_date"].as<date::year_month_day>());
    rhs.set_start_collect_data_day(node["start_collect_data_day"].as<int>());

    rhs.set_total_time(TimeHelpers::days_between(rhs.get_starting_date(), rhs.get_ending_date()));
    return true;
  }
};

#endif  // SIMULATION_TIMEFRAME_H
