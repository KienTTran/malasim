#include <gtest/gtest.h>

#include <memory>

#define private public
#include "Configuration/Config.h"
#undef private

#include "Environment/SeasonalRainfall.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ConfigValidationComprehensiveTest : public ::testing::Test {
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

  Config* config() { return Model::get_config(); }
};

TEST_F(ConfigValidationComprehensiveTest, CoversRemainingDemographicAndSeasonalityChecks) {
  auto& demographic = config()->population_demographic_;
  demographic.initial_age_structure_ = {-1};
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
  demographic.initial_age_structure_ = {1};
  demographic.death_rate_by_age_class_ = {-1.0};
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
  demographic.death_rate_by_age_class_ = {1.0};
  demographic.mortality_when_treatment_fail_by_age_class_ = {-1.0};
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
  demographic.mortality_when_treatment_fail_by_age_class_ = {1.0};
  demographic.artificial_rescaling_of_population_size_ = 0.0;
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);

  config()->seasonality_settings_.set_enable(true);
  config()->seasonality_settings_.set_mode("rainfall");
  auto rainfall = std::make_unique<SeasonalRainfall>();
  rainfall->set_filename("test_seasonality.csv");
  rainfall->set_period(366);
  config()->seasonality_settings_.set_seasonal_rainfall(std::move(rainfall));
  EXPECT_THROW(config()->validate_seasonality_settings(), std::invalid_argument);
}

TEST_F(ConfigValidationComprehensiveTest, CoversMovementAndMosquitoConfigurationChecks) {
  auto circulation = config()->movement_settings_.circulation_info_;
  circulation.number_of_moving_levels_ = 0;
  config()->movement_settings_.circulation_info_ = circulation;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);

  circulation.number_of_moving_levels_ = 1;
  circulation.circulation_percent_ = -1;
  config()->movement_settings_.circulation_info_ = circulation;
  EXPECT_THROW(config()->validate_movement_settings(), std::invalid_argument);

  auto mosquito = config()->mosquito_parameters_.mosquito_config_;
  mosquito.mode_ = "grid_based";
  mosquito.grid_based_.interrupted_feeding_rate_raster_.clear();
  config()->mosquito_parameters_.mosquito_config_ = mosquito;
  EXPECT_THROW(config()->validate_mosquito_parameters(), std::invalid_argument);

  mosquito.mode_ = "location_based";
  mosquito.location_based_.interrupted_feeding_rate_.clear();
  mosquito.location_based_.prmc_size_ = {1};
  config()->mosquito_parameters_.mosquito_config_ = mosquito;
  EXPECT_THROW(config()->validate_mosquito_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationComprehensiveTest, CoversTherapyAndDrugValidationChecks) {
  auto drugs = config()->drug_parameters_.get_drug_db_raw();
  ASSERT_FALSE(drugs.empty());
  auto drug = drugs.begin()->second;
  drug.set_age_specific_drug_concentration_sd({1.0});
  drugs.begin()->second = drug;
  config()->drug_parameters_.set_drug_db_raw(drugs);
  EXPECT_THROW(config()->validate_drug_parameters(), std::invalid_argument);

  drug.set_age_specific_drug_concentration_sd(
      std::vector<double>(config()->population_demographic_.number_of_age_classes_, 1.0));
  drug.set_age_specific_drug_absorption({1.0});
  drugs.begin()->second = drug;
  config()->drug_parameters_.set_drug_db_raw(drugs);
  EXPECT_THROW(config()->validate_drug_parameters(), std::invalid_argument);

  auto therapies = config()->therapy_parameters_.therapy_db_raw_;
  ASSERT_FALSE(therapies.empty());
  auto therapy = therapies.begin()->second;
  therapy.set_drug_ids({drugs.begin()->first});
  therapy.set_dosing_days({-1});
  therapies.begin()->second = therapy;
  config()->therapy_parameters_.therapy_db_raw_ = therapies;
  EXPECT_THROW((void)config()->validate_therapy_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationComprehensiveTest, CoversPopulationEventDateValidation) {
  config()->population_events_.events_raw_.clear();
  PopulationEvents::PopulationEvent event;
  event.name_ = "event";
  event.info_.resize(1);
  event.info_[0].date_ = date::year{1999} / 1 / 1;
  config()->population_events_.events_raw_.push_back(event);
  EXPECT_THROW(config()->validate_population_events(), std::invalid_argument);
}
