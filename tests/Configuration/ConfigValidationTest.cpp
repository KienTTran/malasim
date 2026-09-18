#include <gtest/gtest.h>

#define private public
#include "Configuration/Config.h"
#undef private

#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ConfigValidationTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  Config *config() { return Model::get_config(); }
};

TEST_F(ConfigValidationTest, RejectsInvalidScalarSettings) {
  config()->model_settings_.days_between_stdout_output_ = -1;
  EXPECT_THROW(config()->validate_model_settings(), std::invalid_argument);

  config()->transmission_settings_.set_transmission_parameter(1.1);
  EXPECT_THROW(config()->validate_transmission_settings(), std::invalid_argument);
  config()->transmission_settings_.transmission_parameter_ = 0.5;
  config()->transmission_settings_.p_infection_from_an_infectious_bite_ = -0.1;
  EXPECT_THROW(config()->validate_transmission_settings(), std::invalid_argument);

  config()->population_demographic_.age_structure_.clear();
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
}

TEST_F(ConfigValidationTest, RejectsInvalidTimeframeAndDemography) {
  config()->simulation_timeframe_.ending_date_ = date::year{1999} / 1 / 1;
  EXPECT_THROW(config()->validate_simulation_timeframe(), std::invalid_argument);

  config()->simulation_timeframe_.ending_date_ = date::year{2024} / 1 / 1;
  config()->simulation_timeframe_.start_of_comparison_period_date_ = date::year{2025} / 1 / 1;
  EXPECT_THROW(config()->validate_simulation_timeframe(), std::invalid_argument);

  config()->population_demographic_.age_structure_ = {1, 2};
  config()->population_demographic_.number_of_age_classes_ = 3;
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);

  config()->population_demographic_.number_of_age_classes_ = 2;
  config()->population_demographic_.age_structure_ = {-1, 2};
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
  config()->population_demographic_.age_structure_ = {1, 2};
  config()->population_demographic_.birth_rate_ = -1;
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
}

TEST_F(ConfigValidationTest, RejectsInvalidMovementModels) {
  auto spatial_model = config()->movement_settings_.spatial_model_settings_;
  spatial_model.name_ = "Barabasi";
  spatial_model.barabasi_sm_.r_g_0_ = 0;
  config()->movement_settings_.spatial_model_settings_ = spatial_model;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);

  spatial_model.name_ = "Wesolowski";
  spatial_model.wesolowski_sm_.alpha_ = 0;
  config()->movement_settings_.spatial_model_settings_ = spatial_model;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);

  spatial_model.name_ = "WesolowskiSurface";
  spatial_model.wesolowski_surface_sm_.gamma_ = 0;
  config()->movement_settings_.spatial_model_settings_ = spatial_model;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);

  spatial_model.name_ = "Marshall";
  spatial_model.marshall_sm_.tau_ = 0;
  config()->movement_settings_.spatial_model_settings_ = spatial_model;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);

  spatial_model.name_ = "BurkinaFaso";
  spatial_model.burkina_faso_sm_.penalty_ = 0;
  config()->movement_settings_.spatial_model_settings_ = spatial_model;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);

  auto circulation = config()->movement_settings_.circulation_info_;
  circulation.max_relative_moving_value_ = -1;
  config()->movement_settings_.circulation_info_ = circulation;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);
}

TEST_F(ConfigValidationTest, RejectsInvalidParasiteAndImmuneParameters) {
  auto density = config()->parasite_parameters_.parasite_density_levels_;
  density.log_parasite_density_cured_ = 6;
  config()->parasite_parameters_.parasite_density_levels_ = density;
  EXPECT_THROW(config()->validate_parasite_parameters(), std::invalid_argument);

  density.log_parasite_density_cured_ = 1;
  auto recombination = config()->parasite_parameters_.recombination_parameters_;
  recombination.within_chromosome_recombination_rate_ = 2;
  config()->parasite_parameters_.recombination_parameters_ = recombination;
  EXPECT_THROW(config()->validate_parasite_parameters(), std::invalid_argument);

  config()->immune_system_parameters_.b1_ = -1;
  EXPECT_THROW(config()->validate_immune_system_parameters(), std::invalid_argument);
  config()->immune_system_parameters_.b1_ = 1;
  config()->immune_system_parameters_.midpoint_ = -1;
  EXPECT_THROW(config()->validate_immune_system_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationTest, RejectsInvalidEpidemiologicalParameters) {
  auto &epidemiology = config()->epidemiological_parameters_;
  epidemiology.p_compliance_ = 2;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  epidemiology.p_compliance_ = 0.5;
  epidemiology.min_dosing_days_ = 10;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  epidemiology.p_relapse_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);

  epidemiology.p_relapse_ = 0.2;
  epidemiology.relative_biting_info_.min_relative_biting_value_ = 0;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);

  epidemiology.relative_biting_info_.min_relative_biting_value_ = 2;
  epidemiology.relative_biting_info_.max_relative_biting_value_ = 1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationTest, RejectsInvalidMinimumDosingDaysAndEc50Patterns) {
  auto& epidemiology = config()->epidemiological_parameters_;
  epidemiology.min_dosing_days_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  epidemiology.min_dosing_days_ = 10;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  auto pattern = GenotypeParameters::OverrideEC50Pattern{};
  pattern.set_pattern(std::string(config()->genotype_parameters_.get_mutation_mask().size(), 'A'));
  config()->genotype_parameters_.set_override_ec50_patterns({pattern});
  EXPECT_THROW(config()->validate_genotype_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationTest, RejectsInvalidMosquitoAndPopulationEvents) {
  auto mosquito_config = config()->mosquito_parameters_.mosquito_config_;
  mosquito_config.mode_ = "unknown";
  config()->mosquito_parameters_.mosquito_config_ = mosquito_config;
  EXPECT_THROW(config()->validate_mosquito_parameters(), std::invalid_argument);

  mosquito_config.mode_ = "location_based";
  mosquito_config.location_based_.interrupted_feeding_rate_ = {0.1};
  mosquito_config.location_based_.prmc_size_ = {1, 2};
  config()->mosquito_parameters_.mosquito_config_ = mosquito_config;
  EXPECT_THROW(config()->validate_mosquito_parameters(), std::invalid_argument);

  config()->population_events_.events_raw_.clear();
  PopulationEvents::PopulationEvent event;
  event.name_.clear();
  config()->population_events_.events_raw_.push_back(event);
  EXPECT_THROW(config()->validate_population_events(), std::invalid_argument);
}
