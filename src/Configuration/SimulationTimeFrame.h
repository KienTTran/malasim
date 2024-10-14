#include <yaml-cpp/yaml.h>
#include <date/date.h>
#include <sstream>
#include <stdexcept>

// Class to hold the simulation timeframe data
class SimulationTimeframe {
public:
  // Getters and Setters for starting_date
  [[nodiscard]] const date::year_month_day &get_starting_date() const {
    return starting_date_;
  }

  void set_starting_date(const date::year_month_day &value) {
    starting_date_ = value;
  }

  // Getters and Setters for start_of_comparison_period
  [[nodiscard]] const date::year_month_day &get_start_of_comparison_period() const {
    return start_of_comparison_period_;
  }

  void set_start_of_comparison_period(const date::year_month_day &value) {
    if (value < starting_date_) {
      throw std::invalid_argument("start_of_comparison_period cannot be before starting_date");
    }
    start_of_comparison_period_ = value;
  }

  // Getters and Setters for ending_date
  [[nodiscard]] const date::year_month_day &get_ending_date() const {
    return ending_date_;
  }

  void set_ending_date(const date::year_month_day &value) {
    if (value < start_of_comparison_period_) {
      throw std::invalid_argument("ending_date cannot be before start_of_comparison_period");
    }
    ending_date_ = value;
  }

  // Getters and Setters for start_collect_data_day
  [[nodiscard]] int get_start_collect_data_day() const {
    return start_collect_data_day_;
  }

  void set_start_collect_data_day(int value) {
    if (value < 0) {
      throw std::invalid_argument("start_collect_data_day must be non-negative");
    }
    start_collect_data_day_ = value;
  }

private:
  date::year_month_day starting_date_;
  date::year_month_day start_of_comparison_period_;
  date::year_month_day ending_date_;
  int start_collect_data_day_;
};

// Helper function to parse a date in "YYYY/MM/DD" format using Howard Hinnant's date library
date::year_month_day parseDate(const std::string& date_str) {
    std::istringstream in{date_str};
    date::year_month_day ymd;
    in >> date::parse("%Y/%m/%d", ymd);
    if (in.fail()) {
        throw std::runtime_error("Failed to parse date: " + date_str);
    }
    return ymd;
}

// Specialization of convert for the SimulationTimeframe class
namespace YAML {
template <>
struct convert<SimulationTimeframe> {
  static Node encode(const SimulationTimeframe &rhs) {
    Node node;
    node["starting_date"] = date::format("%Y/%m/%d", rhs.get_starting_date());
    node["start_of_comparison_period"] = date::format("%Y/%m/%d", rhs.get_start_of_comparison_period());
    node["ending_date"] = date::format("%Y/%m/%d", rhs.get_ending_date());
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
    rhs.set_starting_date(parseDate(node["starting_date"].as<std::string>()));
    rhs.set_start_of_comparison_period(parseDate(node["start_of_comparison_period"].as<std::string>()));
    rhs.set_ending_date(parseDate(node["ending_date"].as<std::string>()));
    rhs.set_start_collect_data_day(node["start_collect_data_day"].as<int>());

    return true;
  }
};
}