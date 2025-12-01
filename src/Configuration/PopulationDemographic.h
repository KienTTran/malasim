#ifndef POPULATION_DEMOGRAPHIC_H
#define POPULATION_DEMOGRAPHIC_H

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <stdexcept>
#include <vector>

#include "IConfigData.h"

class PopulationDemographic : public IConfigData {
public:
  // Getters
  [[nodiscard]] int get_number_of_age_classes() const { return number_of_age_classes_; }

  [[nodiscard]] const std::vector<int> &get_age_structure() const { return age_structure_; }
  void set_age_structure(const std::vector<int> &value) {
    if (value.size() <= 0) {
      throw std::invalid_argument("age_structure size must be greater than 0");
    }
    age_structure_ = value;
    number_of_age_classes_ = static_cast<int>(value.size());
  }

  [[nodiscard]] const std::vector<int> &get_initial_age_structure() const {
    return initial_age_structure_;
  }
  void set_initial_age_structure(const std::vector<int> &value) {
    if (value.size() < number_of_age_classes_)
      throw std::invalid_argument("initial_age_structure size must >= number_of_age_classes");
    initial_age_structure_ = value;
  }

  [[nodiscard]] double get_birth_rate() const { return birth_rate_; }
  void set_birth_rate(const double value) {
    if (value < 0) throw std::invalid_argument("birth_rate must be non-negative");
    birth_rate_ = value;
  }

  [[nodiscard]] const std::vector<double> &get_death_rate_by_age_class() const {
    return death_rate_by_age_class_;
  }

  void set_death_rate_by_age_class(const std::vector<double> &value) {
    if (value.size() != number_of_age_classes_)
      throw std::invalid_argument("death_rate_by_age_class size must match number_of_age_classes");
    death_rate_by_age_class_ = value;
  }

  [[nodiscard]] const std::vector<double> &get_mortality_when_treatment_fail_by_age_class() const {
    return mortality_when_treatment_fail_by_age_class_;
  }

  void set_mortality_when_treatment_fail_by_age_class(const std::vector<double> &value) {
    if (value.size() != number_of_age_classes_)
      throw std::invalid_argument(
          "mortality_when_treatment_fail_by_age_class size must match "
          "number_of_age_classes");
    mortality_when_treatment_fail_by_age_class_ = value;
    for (auto& mortality : mortality_when_treatment_fail_by_age_class_) {
      if (mortality < 0)
        throw std::invalid_argument("mortality_when_treatment_fail_by_age_class must be non-negative");
      // spdlog::info("mortality_when_treatment_fail_by_age_class {}", mortality);
    }
  }

  [[nodiscard]] double get_artificial_rescaling_of_population_size() const {
    return artificial_rescaling_of_population_size_;
  }

  void set_artificial_rescaling_of_population_size(const double value) {
    if (value <= 0)
      throw std::invalid_argument("artificial_rescaling_of_population_size must be greater than 0");
    artificial_rescaling_of_population_size_ = value;
  }

  void process_config() override {
    spdlog::info("Processing PopulationDemographic");
    number_of_age_classes_ = static_cast<int>(age_structure_.size());
  }

private:
  int number_of_age_classes_{-1};  // read only
  std::vector<int> age_structure_;
  std::vector<int> initial_age_structure_;
  double birth_rate_{0.0};
  std::vector<double> death_rate_by_age_class_;
  std::vector<double> mortality_when_treatment_fail_by_age_class_;
  double artificial_rescaling_of_population_size_{1.0};
};

template <>
struct YAML::convert<PopulationDemographic> {
  static Node encode(PopulationDemographic &rhs) {
    Node node;
    node["age_structure"] = rhs.get_age_structure();
    node["initial_age_structure"] = rhs.get_initial_age_structure();
    node["birth_rate"] = rhs.get_birth_rate();
    node["death_rate_by_age_class"] = rhs.get_death_rate_by_age_class();
    node["mortality_when_treatment_fail_by_age_class"] =
        rhs.get_mortality_when_treatment_fail_by_age_class();
    node["artificial_rescaling_of_population_size"] =
        rhs.get_artificial_rescaling_of_population_size();
    return node;
  }

  static bool decode(const Node &node, PopulationDemographic &rhs) {
    if (!node["age_structure"]) { throw std::runtime_error("Missing 'age_structure' field."); }
    if (!node["initial_age_structure"]) {
      throw std::runtime_error("Missing 'initial_age_structure' field.");
    }
    if (!node["birth_rate"]) { throw std::runtime_error("Missing 'birth_rate' field."); }
    if (!node["death_rate_by_age_class"]) {
      throw std::runtime_error("Missing 'death_rate_by_age_class' field.");
    }
    if (!node["mortality_when_treatment_fail_by_age_class"]) {
      throw std::runtime_error("Missing 'mortality_when_treatment_fail_by_age_class' field.");
    }
    if (!node["artificial_rescaling_of_population_size"]) {
      throw std::runtime_error("Missing 'artificial_rescaling_of_population_size' field.");
    }
    // Validate and assign age structure vectors
    rhs.set_age_structure(node["age_structure"].as<std::vector<int>>());

    rhs.set_initial_age_structure(node["initial_age_structure"].as<std::vector<int>>());

    rhs.set_birth_rate(node["birth_rate"].as<double>());

    rhs.set_death_rate_by_age_class(node["death_rate_by_age_class"].as<std::vector<double>>());

    rhs.set_mortality_when_treatment_fail_by_age_class(
        node["mortality_when_treatment_fail_by_age_class"].as<std::vector<double>>());

    rhs.set_artificial_rescaling_of_population_size(
        node["artificial_rescaling_of_population_size"].as<double>());

    return true;
  }
};  // namespace YAML

#endif  // POPULATION_DEMOGRAPHIC_H

