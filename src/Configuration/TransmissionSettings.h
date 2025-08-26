#ifndef TRANSMISSION_SETTINGS_H
#define TRANSMISSION_SETTINGS_H

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <stdexcept>

#include "IConfigData.h"

class TransmissionSettings : public IConfigData {
public:
  // Getter for transmission_parameter
  [[nodiscard]] double get_transmission_parameter() const { return transmission_parameter_; }

  // Setter for transmission_parameter with validation
  void set_transmission_parameter(const double value) {
    if (value < 0) throw std::invalid_argument("transmission_parameter must be >= 0");
    transmission_parameter_ = value;
  }

  // Getter for p_infection_from_an_infectious_bite
  [[nodiscard]] double get_p_infection_from_an_infectious_bite() const {
    return p_infection_from_an_infectious_bite_;
  }

  // Setter for p_infection_from_an_infectious_bite with validation
  void set_p_infection_from_an_infectious_bite(const double value) {
    if (value < 0 || value > 1)
      throw std::invalid_argument("p_infection_from_an_infectious_bite must be between 0 and 1");
    p_infection_from_an_infectious_bite_ = value;
  }

  void process_config() override {
    spdlog::info("Processing TransmissionSettings");
    if (get_transmission_parameter() > 0.0) {
      spdlog::info("Using transmission_parameter: {}", get_transmission_parameter());
    }
  }

private:
  double transmission_parameter_ = 0.55;
  double p_infection_from_an_infectious_bite_ = 0.1;
};

template <>
struct YAML::convert<TransmissionSettings> {
  static Node encode(const TransmissionSettings &rhs) {
    Node node;
    node["transmission_parameter"] = rhs.get_transmission_parameter();
    node["p_infection_from_an_infectious_bite"] = rhs.get_p_infection_from_an_infectious_bite();
    return node;
  }

  static bool decode(const Node &node, TransmissionSettings &rhs) {
    if (!node["transmission_parameter"]) {
      throw std::runtime_error("Missing 'transmission_parameter' field.");
    }
    if (!node["p_infection_from_an_infectious_bite"]) {
      throw std::runtime_error("Missing 'p_infection_from_an_infectious_bite' field.");
    }

    rhs.set_transmission_parameter(node["transmission_parameter"].as<double>());
    rhs.set_p_infection_from_an_infectious_bite(
        node["p_infection_from_an_infectious_bite"].as<double>());
    return true;
  }
};  // namespace YAML

#endif  // TRANSMISSION_SETTINGS_H

