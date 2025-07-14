#ifndef IMMUNESYSTEMPARAMETERS_H
#define IMMUNESYSTEMPARAMETERS_H
#include <spdlog/spdlog.h>

#include "IConfigData.h"
#include "Utils/Helpers/NumberHelpers.h"

class ImmuneSystemParameters : public IConfigData {
public:
  // Getters and Setters
  [[nodiscard]] double get_b1() const { return b1_; }
  void set_b1(const double value) { b1_ = value; }

  [[nodiscard]] double get_b2() const { return b2_; }
  void set_b2(const double value) { b2_ = value; }

  [[nodiscard]] int get_duration_for_naive() const { return duration_for_naive_; }
  void set_duration_for_naive(const int value) {
    if (value < 0) throw std::invalid_argument("duration_for_naive must be non-negative");
    duration_for_naive_ = value;
  }

  [[nodiscard]] int get_duration_for_fully_immune() const { return duration_for_fully_immune_; }
  void set_duration_for_fully_immune(const int value) {
    if (value < 0) throw std::invalid_argument("duration_for_fully_immune must be non-negative");
    duration_for_fully_immune_ = value;
  }

  [[nodiscard]] double get_mean_initial_condition() const { return mean_initial_condition_; }
  void set_mean_initial_condition(const double value) { mean_initial_condition_ = value; }

  [[nodiscard]] double get_sd_initial_condition() const { return sd_initial_condition_; }
  void set_sd_initial_condition(const double value) { sd_initial_condition_ = value; }

  [[nodiscard]] double get_immune_inflation_rate() const { return immune_inflation_rate_; }
  void set_immune_inflation_rate(const double value) { immune_inflation_rate_ = value; }

  [[nodiscard]] double get_min_clinical_probability() const { return min_clinical_probability_; }
  void set_min_clinical_probability(const double value) {
    if (value < 0 || value > 1)
      throw std::invalid_argument("min_clinical_probability must be between 0 and 1");
    min_clinical_probability_ = value;
  }

  [[nodiscard]] double get_max_clinical_probability() const { return max_clinical_probability_; }
  void set_max_clinical_probability(const double value) {
    if (value < 0 || value > 1)
      throw std::invalid_argument("max_clinical_probability must be between 0 and 1");
    max_clinical_probability_ = value;
  }

  [[nodiscard]] double get_immune_effect_on_progression_to_clinical() const {
    return immune_effect_on_progression_to_clinical_;
  }
  void set_immune_effect_on_progression_to_clinical(const double value) {
    immune_effect_on_progression_to_clinical_ = value;
  }

  [[nodiscard]] int get_age_mature_immunity() const { return age_mature_immunity_; }
  void set_age_mature_immunity(const int value) {
    if (value < 0) throw std::invalid_argument("age_mature_immunity must be non-negative");
    age_mature_immunity_ = value;
  }

  [[nodiscard]] double get_factor_effect_age_mature_immunity() const {
    return factor_effect_age_mature_immunity_;
  }
  void set_factor_effect_age_mature_immunity(const double value) {
    factor_effect_age_mature_immunity_ = value;
  }

  [[nodiscard]] double get_midpoint() const { return midpoint_; }
  void set_midpoint(const double value) { midpoint_ = value; }

  void process_config() override {}

  void process_config_with_parasite_density(const double log_parasite_density_asymptomatic,
                                            const double log_parasite_density_cured) {
    spdlog::info("Processing ImmuneSystemParameters");
    acquire_rate = b1_;
    decay_rate = b2_;
    duration_for_fully_immune = duration_for_fully_immune_;
    duration_for_naive = duration_for_naive_;
    if (NumberHelpers::is_zero(sd_initial_condition_)) {
      alpha_immune = mean_initial_condition_;
      beta_immune = 0.0;
    } else {
      alpha_immune = mean_initial_condition_ * mean_initial_condition_
                         * (1 - mean_initial_condition_)
                         / (sd_initial_condition_ * sd_initial_condition_)
                     - mean_initial_condition_;
      beta_immune = alpha_immune / mean_initial_condition_ - alpha_immune;
    }

    immune_inflation_rate = immune_inflation_rate_;

    max_clinical_probability = max_clinical_probability_;

    immune_effect_on_progression_to_clinical = immune_effect_on_progression_to_clinical_;

    age_mature_immunity = age_mature_immunity_;
    factor_effect_age_mature_immunity = factor_effect_age_mature_immunity_;

    midpoint = midpoint_;

    // implement inflation rate
    double acR = acquire_rate;
    acquire_rate_by_age.clear();
    for (int age = 0; age <= 80; age++) {
      double factor = 1;
      if (age < age_mature_immunity) {
        factor = age == 0 ? 0.5 : age;

        factor = factor / age_mature_immunity;
        factor = pow(factor, factor_effect_age_mature_immunity);
      }

      acquire_rate_by_age.push_back(factor * acR);

      acR *= 1 + immune_inflation_rate;
    }
    assert(acquire_rate_by_age.size() == 81);

    c_min = pow(10, -(log_parasite_density_asymptomatic - log_parasite_density_cured)
                        / duration_for_fully_immune);
    c_max = pow(
        10, -(log_parasite_density_asymptomatic - log_parasite_density_cured) / duration_for_naive);
  }

private:
  double b1_ = 0.00125;
  double b2_ = 0.0025;
  int duration_for_naive_ = 300;
  int duration_for_fully_immune_ = 60;
  double mean_initial_condition_ = 0.1;
  double sd_initial_condition_ = 0.1;
  double immune_inflation_rate_ = 0.01;
  double min_clinical_probability_ = 0.05;
  double max_clinical_probability_ = 0.99;
  double immune_effect_on_progression_to_clinical_ = 5.4;
  int age_mature_immunity_ = 10;
  double factor_effect_age_mature_immunity_ = 0.3;
  double midpoint_ = 0.4;

public:
  double acquire_rate{-1};
  std::vector<double> acquire_rate_by_age;
  double decay_rate{-1};

  double duration_for_fully_immune{-1};
  double duration_for_naive{-1};

  double immune_inflation_rate{-1};

  double max_clinical_probability{-1};

  // Slope of the sigmoidal prob-v-immunity function, z-value
  double immune_effect_on_progression_to_clinical{-1};

  // The midpoint of the sigmoidal prob-v-immunity function, recommended default
  // 0.4
  double midpoint{-1};

  double c_min{-1};
  double c_max{-1};

  double alpha_immune{-1};
  double beta_immune{-1};

  double age_mature_immunity{-1};

  // Parameter kappa in supplement of 2015 LGH paper
  double factor_effect_age_mature_immunity{-1};
};

// ImmuneSystemParameters YAML conversion
template <>
struct YAML::convert<ImmuneSystemParameters> {
  static Node encode(const ImmuneSystemParameters &rhs) {
    Node node;
    node["b1"] = rhs.get_b1();
    node["b2"] = rhs.get_b2();
    node["duration_for_naive"] = rhs.get_duration_for_naive();
    node["duration_for_fully_immune"] = rhs.get_duration_for_fully_immune();
    node["mean_initial_condition"] = rhs.get_mean_initial_condition();
    node["sd_initial_condition"] = rhs.get_sd_initial_condition();
    node["immune_inflation_rate"] = rhs.get_immune_inflation_rate();
    node["min_clinical_probability"] = rhs.get_min_clinical_probability();
    node["max_clinical_probability"] = rhs.get_max_clinical_probability();
    node["immune_effect_on_progression_to_clinical"] =
        rhs.get_immune_effect_on_progression_to_clinical();
    node["age_mature_immunity"] = rhs.get_age_mature_immunity();
    node["factor_effect_age_mature_immunity"] = rhs.get_factor_effect_age_mature_immunity();
    node["midpoint"] = rhs.get_midpoint();
    return node;
  }

  static bool decode(const Node &node, ImmuneSystemParameters &rhs) {
    if (!node["b1"] || !node["b2"] || !node["duration_for_naive"]
        || !node["duration_for_fully_immune"] || !node["mean_initial_condition"]
        || !node["sd_initial_condition"] || !node["immune_inflation_rate"]
        || !node["min_clinical_probability"] || !node["max_clinical_probability"]
        || !node["immune_effect_on_progression_to_clinical"] || !node["age_mature_immunity"]
        || !node["factor_effect_age_mature_immunity"] || !node["midpoint"]) {
      throw std::runtime_error("Missing fields in ImmuneSystemParameters");
    }

    rhs.set_b1(node["b1"].as<double>());
    rhs.set_b2(node["b2"].as<double>());
    rhs.set_duration_for_naive(node["duration_for_naive"].as<int>());
    rhs.set_duration_for_fully_immune(node["duration_for_fully_immune"].as<int>());
    rhs.set_mean_initial_condition(node["mean_initial_condition"].as<double>());
    rhs.set_sd_initial_condition(node["sd_initial_condition"].as<double>());
    rhs.set_immune_inflation_rate(node["immune_inflation_rate"].as<double>());
    rhs.set_min_clinical_probability(node["min_clinical_probability"].as<double>());
    rhs.set_max_clinical_probability(node["max_clinical_probability"].as<double>());
    rhs.set_immune_effect_on_progression_to_clinical(
        node["immune_effect_on_progression_to_clinical"].as<double>());
    rhs.set_age_mature_immunity(node["age_mature_immunity"].as<int>());
    rhs.set_factor_effect_age_mature_immunity(
        node["factor_effect_age_mature_immunity"].as<double>());
    rhs.set_midpoint(node["midpoint"].as<double>());
    return true;
  }
};
#endif  // IMMUNESYSTEMPARAMETERS_H
