#include <gtest/gtest.h>

#include "Simulation/Model.h"
#include "Spatial/GIS/SpatialData.h"
#include "TestHelpers.h"

class SpatialDataYamlFallbackCoverageTest : public ::testing::Test,
                                             protected SpatialDataTestHelper {
protected:
  void SetUp() override { SpatialDataTestHelper::SetUp(); }
  void TearDown() override { SpatialDataTestHelper::TearDown(); }
};

TEST_F(SpatialDataYamlFallbackCoverageTest, LoadsLocationDataAndTreatmentFromYamlWhenRastersAbsent) {
  auto node = createBasicNode();
  node.remove("population_raster");
  node["location_raster"] = "test_population.asc";

  Model::get_config()->get_spatial_settings().location_db().clear();
  Model::get_config()->get_spatial_settings().set_number_of_locations(0);
  SpatialData yaml_data(&Model::get_config()->get_spatial_settings());
  ASSERT_NO_THROW(yaml_data.process_config(node));

  const auto &locations = Model::get_config()->get_spatial_settings().location_db();
  ASSERT_EQ(locations.size(), 8U);
  EXPECT_EQ(locations[0].population_size, 100);
  EXPECT_DOUBLE_EQ(locations[0].beta, 0.5);
  EXPECT_FLOAT_EQ(locations[0].p_treatment_under_5, 0.5F);
  EXPECT_FLOAT_EQ(locations[0].p_treatment_over_5, 0.5F);
}
