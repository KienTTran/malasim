#include <gtest/gtest.h>

#include "Configuration/SpatialSettings/SpatialSettings.h"

TEST(SpatialSettingsAccessorCoverageTest, StoresScalarNodeLocationAndDistanceState) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::LOCATION_BASED_MODE);
  settings.set_number_of_locations(4);
  YAML::Node node;
  node["mode"] = SpatialSettings::LOCATION_BASED_MODE;
  settings.set_node(node);
  Spatial::Location location;
  location.id = 3;
  settings.set_location_db({location});
  settings.set_distance_provider(std::make_unique<DenseDistanceProvider>(
      std::vector<std::vector<double>>{{0.0}}));

  const auto &const_settings = settings;
  EXPECT_EQ(const_settings.get_mode(), SpatialSettings::LOCATION_BASED_MODE);
  EXPECT_EQ(const_settings.get_number_of_locations(), 4U);
  EXPECT_TRUE(const_settings.get_node());
  EXPECT_EQ(settings.location_db().size(), 1U);
  EXPECT_EQ(settings.location_db().front().id, 3);
  ASSERT_NE(settings.get_distance_provider(), nullptr);
  EXPECT_DOUBLE_EQ(settings.get_distance_provider()->distance(0, 0), 0.0);
  ASSERT_NE(const_settings.get_distance_provider(), nullptr);
  EXPECT_DOUBLE_EQ(const_settings.get_distance_provider()->distance(0, 0), 0.0);
  EXPECT_EQ(settings.spatial_data(), nullptr);
}

TEST(SpatialSettingsAccessorCoverageTest, EncodesGridBasedFieldsAndBoundaries) {
  SpatialSettings::GridBased grid;
  grid.population_raster = "population.asc";
  grid.p_treatment_under_5_raster = "under5.asc";
  grid.p_treatment_over_5_raster = "over5.asc";
  grid.beta_raster = "beta.asc";
  grid.ecoclimatic_raster = "eco.asc";
  grid.cell_size = 2.5;
  grid.age_distribution_by_location = {{1.0}};
  grid.administrative_boundaries.push_back({"district", "district.asc"});

  const auto node = YAML::convert<SpatialSettings::GridBased>::encode(grid);
  EXPECT_EQ(node["population_raster"].as<std::string>(), "population.asc");
  EXPECT_EQ(node["administrative_boundaries"][0]["name"].as<std::string>(), "district");
  EXPECT_DOUBLE_EQ(node["cell_size"].as<double>(), 2.5);
}
