#include <gtest/gtest.h>

#include "Configuration/SpatialSettings/SpatialSettings.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class SpatialSettingsCrossValidationTest : public ::testing::Test {
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
};

TEST_F(SpatialSettingsCrossValidationTest, RejectsUnknownMode) {
  SpatialSettings settings;
  settings.set_mode("unsupported");
  EXPECT_THROW(settings.cross_validate(), std::invalid_argument);
}

TEST_F(SpatialSettingsCrossValidationTest, RejectsIncompleteLocationBasedSettings) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::LOCATION_BASED_MODE);
  settings.set_node(YAML::Load("population_size_by_location: []"));
  EXPECT_THROW(settings.cross_validate(), std::runtime_error);
}

TEST_F(SpatialSettingsCrossValidationTest, RejectsMismatchedLocationAgeDistribution) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::LOCATION_BASED_MODE);
  YAML::Node node;
  node["population_size_by_location"] = std::vector<int>{10};
  node["location_info"] = YAML::Load("[[0, 0.0, 0.0]]");
  node["age_distribution_by_location"] = YAML::Load("[[1.0]]");
  node["p_treatment_under_5_by_location"] = std::vector<double>{0.1};
  node["p_treatment_over_5_by_location"] = std::vector<double>{0.2};
  node["beta_by_location"] = std::vector<double>{0.3};
  settings.set_node(node);
  EXPECT_THROW(settings.cross_validate(), std::invalid_argument);
}

TEST_F(SpatialSettingsCrossValidationTest, RejectsMissingGridRasterPath) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::GRID_BASED_MODE);
  YAML::Node node;
  node["population_raster"] = "";
  node["beta_raster"] = "beta.asc";
  node["p_treatment_under_5_raster"] = "under5.asc";
  node["p_treatment_over_5_raster"] = "over5.asc";
  node["administrative_boundaries"] = YAML::Load("[]");
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = YAML::Load("[[1.0]]");
  settings.set_node(node);
  EXPECT_THROW(settings.cross_validate(), std::invalid_argument);
}

TEST_F(SpatialSettingsCrossValidationTest, RejectsMultipleGridAgeDistributions) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::GRID_BASED_MODE);
  YAML::Node node;
  node["population_raster"] = "population.asc";
  node["beta_raster"] = "beta.asc";
  node["p_treatment_under_5_raster"] = "under5.asc";
  node["p_treatment_over_5_raster"] = "over5.asc";
  node["administrative_boundaries"] = YAML::Load("[]");
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = YAML::Load("[[1.0], [1.0]]");
  settings.set_node(node);
  EXPECT_THROW(settings.cross_validate(), std::invalid_argument);
}

TEST_F(SpatialSettingsCrossValidationTest, RejectsGridAgeDistributionWithWrongAgeCount) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::GRID_BASED_MODE);
  YAML::Node node;
  node["population_raster"] = "population.asc";
  node["beta_raster"] = "beta.asc";
  node["p_treatment_under_5_raster"] = "under5.asc";
  node["p_treatment_over_5_raster"] = "over5.asc";
  node["administrative_boundaries"] = YAML::Load("[]");
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = YAML::Load("[[1.0]]");
  settings.set_node(node);
  EXPECT_THROW(settings.cross_validate(), std::invalid_argument);
}

TEST_F(SpatialSettingsCrossValidationTest, RejectsLocationAgeDistributionCountMismatch) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::LOCATION_BASED_MODE);
  YAML::Node node;
  node["population_size_by_location"] = std::vector<int>{10, 10};
  node["location_info"] = YAML::Load("[[0, 0.0, 0.0], [1, 1.0, 1.0]]");
  node["age_distribution_by_location"] = YAML::Load("[[1.0], [1.0], [1.0]]");
  node["p_treatment_under_5_by_location"] = std::vector<double>{0.1};
  node["p_treatment_over_5_by_location"] = std::vector<double>{0.2};
  node["beta_by_location"] = std::vector<double>{0.3};
  settings.set_node(node);
  EXPECT_THROW(settings.cross_validate(), std::invalid_argument);
}
