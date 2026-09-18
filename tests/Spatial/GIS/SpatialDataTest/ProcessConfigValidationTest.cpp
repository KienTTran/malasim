#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Configuration/Config.h"
#include "Spatial/GIS/SpatialData.h"
#include "TestHelpers.h"

class SpatialDataProcessConfigValidationTest : public ::testing::Test,
                                                protected SpatialDataTestHelper {
protected:
  void SetUp() override { SpatialDataTestHelper::SetUp(); }
  void TearDown() override { SpatialDataTestHelper::TearDown(); }
};

TEST_F(SpatialDataProcessConfigValidationTest, RejectsMissingCellSizeBeforeLoadingFiles) {
  YAML::Node node;
  EXPECT_THROW(spatial_data->process_config(node), std::runtime_error);
}

TEST_F(SpatialDataProcessConfigValidationTest, RejectsASecondProcessWithExistingLocationDatabase) {
  Model::get_config()->get_spatial_settings().set_number_of_locations(1);
  EXPECT_THROW(spatial_data->process_config(createBasicNode()), std::runtime_error);
}

TEST(SpatialDataProcessConfigNoFixtureTest, RejectsConfigurationWithoutAnyRaster) {
  SpatialSettings settings;
  SpatialData spatial_data(&settings);
  YAML::Node node;
  node["cell_size"] = 1.0;
  EXPECT_THROW(spatial_data.process_config(node), std::runtime_error);
}

TEST_F(SpatialDataProcessConfigValidationTest, RejectsMalformedAdministrativeBoundarySection) {
  auto node = createBasicNode();
  node["administrative_boundaries"] = YAML::Load("{name: district}");
  SpatialSettings settings;
  SpatialData malformed(&settings);
  EXPECT_THROW(malformed.process_config(node), std::runtime_error);

  node["administrative_boundaries"] = YAML::Load("- {name: district}");
  SpatialData missing_raster(&settings);
  EXPECT_THROW(missing_raster.process_config(node), std::runtime_error);
}

TEST_F(SpatialDataProcessConfigValidationTest, RejectsMissingYamlLocationInputs) {
  auto node = createBasicNode();
  node.remove("age_distribution_by_location");
  SpatialSettings age_settings;
  SpatialData missing_age(&age_settings);
  EXPECT_THROW(missing_age.process_config(node), std::runtime_error);

  node = createBasicNode();
  node.remove("p_treatment_for_under_5_by_location");
  SpatialSettings under5_settings;
  SpatialData missing_under5(&under5_settings);
  EXPECT_THROW(missing_under5.process_config(node), std::runtime_error);

  node = createBasicNode();
  node.remove("p_treatment_for_over_5_by_location");
  SpatialSettings over5_settings;
  SpatialData missing_over5(&over5_settings);
  EXPECT_THROW(missing_over5.process_config(node), std::runtime_error);

  node = createBasicNode();
  node.remove("population_raster");
  node.remove("population_size_by_location");
  SpatialSettings population_settings;
  SpatialData missing_population(&population_settings);
  EXPECT_THROW(missing_population.process_config(node), std::runtime_error);
}
