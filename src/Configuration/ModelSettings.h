#ifndef MODEL_SETTINGS_H
#define MODEL_SETTINGS_H

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

#include "IConfigData.h"
#include "Utils/YamlFile.hxx"
#include "Utils/Random.h"

class ModelSettings : public IConfigData {
public:
  // Getters
  [[nodiscard]] int get_days_between_stdout_output() const {
    return days_between_stdout_output_;
  }
  // Setters with validation
  void set_days_between_stdout_output(int value) {
    if (value <= 0)
      throw std::invalid_argument(
          "days_between_stdout_output must be greater than 0");
    days_between_stdout_output_ = value;
  }
  [[nodiscard]] long get_initial_seed_number() const {
    return initial_seed_number_;
  }
  void set_initial_seed_number(long value) {
    if (value < 0) {
      spdlog::info("Using random seed number");
    }
    else {
      initial_seed_number_ = value;
      spdlog::info("Using predefined seed number: {}", value);
    }
  }
  [[nodiscard]] bool get_record_genome_db() const { return record_genome_db_; }
  void set_record_genome_db(bool value) { record_genome_db_ = value; }

  bool get_cell_level_reporting() const { return cell_level_reporting_; }
  void set_cell_level_reporting(bool value) { cell_level_reporting_ = value; }

  void process_config() override {
    spdlog::info("Processing ModelSettings");
  }

private:
  int days_between_stdout_output_;
  long initial_seed_number_;
  bool record_genome_db_;
  bool cell_level_reporting_;
};

namespace YAML {
template <>
struct convert<ModelSettings> {
  static Node encode(const ModelSettings &rhs) {
    Node node;
    node["days_between_stdout_output"] = rhs.get_days_between_stdout_output();
    node["initial_seed_number"] = rhs.get_initial_seed_number();
    node["record_genome_db"] = rhs.get_record_genome_db();
    node["cell_level_reporting"] = rhs.get_cell_level_reporting();
    return node;
  }

  static bool decode(const Node &node, ModelSettings &rhs) {
    if (!node["days_between_stdout_output"]) {
      throw std::runtime_error("Missing 'days_between_stdout_output' field.");
    }
    if (!node["initial_seed_number"]) {
      throw std::runtime_error("Missing 'initial_seed_number' field.");
    }
    if (!node["record_genome_db"]) {
      throw std::runtime_error("Missing 'record_genome_db' field.");
    }
    if (!node["cell_level_reporting"]) {
      throw std::runtime_error("Missing 'cell_level_reporting' field.");
    }

    rhs.set_days_between_stdout_output(
        node["days_between_stdout_output"].as<int>());
    rhs.set_initial_seed_number(node["initial_seed_number"].as<long>());
    rhs.set_record_genome_db(node["record_genome_db"].as<bool>());
    rhs.set_cell_level_reporting(node["cell_level_reporting"].as<bool>());
    return true;
  }
};
}  // namespace YAML

#endif  // MODEL_SETTINGS_H

