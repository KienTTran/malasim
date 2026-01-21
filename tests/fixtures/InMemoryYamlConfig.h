#ifndef IN_MEMORY_YAML_CONFIG_H
#define IN_MEMORY_YAML_CONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>

namespace test_fixtures {

/**
 * @brief Create YAML nodes from in-memory strings for testing configuration parsing
 * without depending on external files.
 */
namespace yaml_configs {

/**
 * @brief Minimal valid spatial settings in grid_based mode
 */
inline std::string minimal_grid_based_spatial() {
  return R"(
mode: grid_based
grid_based:
  population_raster: "test_pop.asc"
  administrative_boundaries:
    - name: "district"
      raster: "test_district.asc"
  p_treatment_under_5_raster: "test_treatment.asc"
  p_treatment_over_5_raster: "test_treatment.asc"
  beta_raster: "test_beta.asc"
  cell_size: 5.0
  age_distribution_by_location:
    - [0.0378, 0.0378, 0.0378, 0.0378, 0.0282, 0.0282, 0.0282, 0.0282, 0.0282, 0.029, 0.029, 0.029, 0.029, 0.029, 0.169, 0.134, 0.106, 0.066, 0.053, 0.035, 0.0]
)";
}

/**
 * @brief Minimal valid spatial settings in location_based mode
 */
inline std::string minimal_location_based_spatial() {
  return R"(
mode: location_based
location_based:
  location_info:
    - [1, 10.0, 20.0]
    - [2, 15.0, 25.0]
  age_distribution_by_location:
    - [0.0378, 0.0378, 0.0378, 0.0378, 0.0282, 0.0282, 0.0282, 0.0282, 0.0282, 0.029, 0.029, 0.029, 0.029, 0.029, 0.169, 0.134, 0.106, 0.066, 0.053, 0.035, 0.0]
  p_treatment_under_5_by_location: [0.5, 0.6]
  p_treatment_over_5_by_location: [0.4, 0.5]
  beta_by_location: [0.05, 0.06]
  population_size_by_location: [1000, 1200]
)";
}

/**
 * @brief Minimal valid population demographic configuration
 */
inline std::string minimal_population_demographic() {
  return R"(
age_structure: [5, 15, 30, 50, 70, 90]
mortality_when_treatment_fail_by_age_class: [0.4, 0.4, 0.4, 0.4, 0.4, 0.4]
death_rate_by_age_class: [0.2, 0.2, 0.2, 0.2, 0.2, 0.2]
)";
}

/**
 * @brief Minimal valid epidemiological parameters
 */
inline std::string minimal_epidemiological_parameters() {
  return R"(
days_to_clinical_under_five: 5
days_to_clinical_over_five: 7
days_mature_gametocyte_under_five: 10
days_mature_gametocyte_over_five: 14
)";
}

/**
 * @brief Minimal valid drug parameters
 */
inline std::string minimal_drug_parameters() {
  return R"(
drug_id: 0
name: "TestDrug"
drug_half_life: 100.0
maximum_parasite_killing_rate: 0.9
n: 4.0
k: 0.5
cut_off_percent: 10.0
)";
}

/**
 * @brief Minimal valid therapy configuration
 */
inline std::string minimal_therapy() {
  return R"(
id: 0
drug_ids: [0]
dosing_days: [3]
)";
}

/**
 * @brief Minimal valid seasonality settings
 */
inline std::string minimal_seasonality() {
  return R"(
enable: true
mode: "monthly"
a: [0.8, 0.9, 1.0, 1.1, 1.2, 1.1, 1.0, 0.9, 0.8, 0.7, 0.6, 0.7]
phi: 0.5
min_value: 0.1
)";
}

/**
 * @brief Minimal valid simulation timeframe
 */
inline std::string minimal_simulation_timeframe() {
  return R"(
total_time: 1000
start_day_of_year: 1
start_year: 2000
)";
}

/**
 * @brief Minimal valid model settings
 */
inline std::string minimal_model_settings() {
  return R"(
cell_level_reporting: true
age_bracket_reporting: [1, 5, 15, 100]
initial_seed_number: 0
record_genome_db: false
)";
}

/**
 * @brief Helper to parse YAML string to node
 */
inline YAML::Node parse_yaml(const std::string& yaml_str) {
  return YAML::Load(yaml_str);
}

}  // namespace yaml_configs
}  // namespace test_fixtures

#endif  // IN_MEMORY_YAML_CONFIG_H
