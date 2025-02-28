// Config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include "DrugParameters.h"
#include "EpidemiologicalParameters.h"
#include "GenotypeParameters.h"
#include "ImmuneSystemParameters.h"
#include "ModelSettings.h"
#include "MosquitoParameters.h"
#include "MovementSettings.h"
#include "ParasiteParameters.h"
#include "PopulationDemographic.h"
#include "PopulationEvents.h"
#include "RaptSettings.h"
#include "TransmissionSettings.h"
#include "SimulationTimeframe.h"
#include "SpatialSettings.h"
#include "SeasonalitySettings.h"
#include "StrategyParameters.h"
#include "TherapyParameters.h"

class Config{
public:
  //disallow copy, assign and move
  Config(const Config&) = delete;
  void operator=(const Config&) = delete;
  Config(Config&&) = delete;
  Config& operator=(Config&&) = delete;

  // Constructor and Destructor
  Config() = default;
  ~Config() = default;

  // Load configuration from a YAML file
  bool load(const std::string &filename);

  // Reload configuration (useful for dynamic updates)
  void reload();

  // Validate all cross-field validations
  void validate_all_cross_field_validations();

  // Getters for entire configuration structures
  [[nodiscard]] const ModelSettings &get_model_settings() const {
    return model_settings_;
  }
  [[nodiscard]] SimulationTimeframe &get_simulation_timeframe() {
    return simulation_timeframe_;
  }
  [[nodiscard]] const TransmissionSettings &get_transmission_settings() const {
    return transmission_settings_;
  }
  [[nodiscard]] const PopulationDemographic &get_population_demographic()
      const {
    return population_demographic_;
  }
  [[nodiscard]] SpatialSettings &get_spatial_settings() {
    /* no const here because Spatial Data class will need to access and modify later */
    return spatial_settings_;
  }
  [[nodiscard]] SeasonalitySettings &get_seasonality_settings() {
    return seasonality_settings_;
  }
  [[nodiscard]] MovementSettings &get_movement_settings() {
    return movement_settings_;
  }
  [[nodiscard]] const ParasiteParameters &get_parasite_parameters() const {
    return parasite_parameters_;
  }
  [[nodiscard]] const ImmuneSystemParameters &get_immune_system_parameters()
      const {
    return immune_system_parameters_;
  }
  [[nodiscard]] GenotypeParameters &get_genotype_parameters()
      {
    return genotype_parameters_;
  }
  [[nodiscard]] const DrugParameters &get_drug_parameters()
      const {
    return drug_parameters_;
  }
  [[nodiscard]] const TherapyParameters &get_therapy_parameters()
      const {
    return therapy_parameters_;
  }
  [[nodiscard]] StrategyParameters &get_strategy_parameters()
      {
    return strategy_parameters_;
  }
  [[nodiscard]] const EpidemiologicalParameters &get_epidemiological_parameters()
      const {
    return epidemiological_parameters_;
  }
  [[nodiscard]] MosquitoParameters &get_mosquito_parameters()
      {
    return mosquito_parameters_;
  }
  [[nodiscard]] PopulationEvents &get_population_events()
      {
    return population_events_;
  }
  [[nodiscard]] RaptSettings &get_rapt_settings() {
    return rapt_settings_;
  }

private:
  // Template method for getting a field
  template <typename T>
  [[nodiscard]] const T &get_field(const T &field) const {
    return field;
  }

  // Template method for setting a field
  template <typename T>
  void set_field(T &field, const T &value) {
    field = value;
  }

  // Configuration File Path
  std::string config_file_path_;
  Model *model_;

  ModelSettings model_settings_;
  TransmissionSettings transmission_settings_;
  PopulationDemographic population_demographic_;
  SimulationTimeframe simulation_timeframe_;
  SpatialSettings spatial_settings_;
  SeasonalitySettings seasonality_settings_;
  MovementSettings movement_settings_;
  ParasiteParameters parasite_parameters_;
  ImmuneSystemParameters immune_system_parameters_;
  GenotypeParameters genotype_parameters_;
  DrugParameters drug_parameters_;
  TherapyParameters therapy_parameters_;
  StrategyParameters strategy_parameters_;
  EpidemiologicalParameters epidemiological_parameters_;
  MosquitoParameters mosquito_parameters_;
  PopulationEvents population_events_;
  RaptSettings rapt_settings_;
};

#endif  // CONFIG_H
