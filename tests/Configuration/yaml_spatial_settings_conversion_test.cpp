#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Configuration/SpatialSettings/SpatialSettings.h"
#include "fixtures/InMemoryYamlConfig.h"

// Helper function to compare std::vector<double>
template <typename T>
void CompareVectors(const std::vector<T> &expected, const std::vector<T> &actual, const int N) {
  for (size_t i = 0; i < N; ++i) { EXPECT_EQ(expected[i], actual[i]); }
}

class SpatialSettingsTest : public ::testing::Test {
protected:
  SpatialSettings default_settings;
  SpatialSettings::GridBased grid;

  void SetUp() override {
    default_settings.set_mode("grid_based");

    // Use mock raster file paths instead of sample_inputs
    grid.population_raster = "test_pop.asc";
    grid.p_treatment_under_5_raster = "test_treatment.asc";
    grid.p_treatment_over_5_raster = "test_treatment.asc";
    grid.beta_raster = "test_beta.asc";
    grid.cell_size = 5.0;
    grid.age_distribution_by_location = {{0.0378, 0.0378, 0.0378, 0.0378, 0.0282, 0.0282, 0.0282,
                                          0.0282, 0.0282, 0.029,  0.029,  0.029,  0.029,  0.029,
                                          0.169,  0.134,  0.106,  0.066,  0.053,  0.035,  0.0}};
  }
};

// Test encoding functionality for SpatialSettings
TEST_F(SpatialSettingsTest, EncodeSpatialSettings) {
  YAML::Node node = YAML::convert<SpatialSettings>::encode(default_settings);

  EXPECT_EQ(node["mode"].as<std::string>(), default_settings.get_mode());

  // Note: SpatialSettings::encode only encodes the mode and the stored YAML node.
  // It does not re-encode the members of GridBased/LocationBased structs.
  // We should test the encode functions for GridBased/LocationBased directly.
  // This test as written is not very meaningful for the nested data.
  // We will remove the checks for grid members here and add specific encode tests later.

  // EXPECT_EQ(node["grid_based"]["population_raster"].as<std::string>(), grid.population_raster);
  // ... remove other grid member checks ...
}

// Test decoding functionality for SpatialSettings mode and node extraction
TEST(SpatialSettingsDecodeTest, DecodeSpatialSettingsModeAndNode) {
  YAML::Node node;
  node["mode"] = SpatialSettings::GRID_BASED_MODE;
  YAML::Node grid_node;  // Create a node for grid_based settings
  grid_node["population_raster"] = "pop.asc";
  node[SpatialSettings::GRID_BASED_MODE] = grid_node;

  SpatialSettings decoded_settings;
  EXPECT_NO_THROW(YAML::convert<SpatialSettings>::decode(node, decoded_settings));
  EXPECT_EQ(decoded_settings.get_mode(), SpatialSettings::GRID_BASED_MODE);
  ASSERT_TRUE(decoded_settings.get_node());  // Check if node was stored
  EXPECT_EQ(decoded_settings.get_node()["population_raster"].as<std::string>(), "pop.asc");

  // Test location_based mode
  node = YAML::Node();  // Reset node
  node["mode"] = SpatialSettings::LOCATION_BASED_MODE;
  YAML::Node loc_node;  // Create a node for location_based settings
  loc_node["beta_by_location"] = std::vector<double>{0.1};
  node[SpatialSettings::LOCATION_BASED_MODE] = loc_node;

  EXPECT_NO_THROW(YAML::convert<SpatialSettings>::decode(node, decoded_settings));
  EXPECT_EQ(decoded_settings.get_mode(), SpatialSettings::LOCATION_BASED_MODE);
  ASSERT_TRUE(decoded_settings.get_node());  // Check if node was stored
  EXPECT_EQ(decoded_settings.get_node()["beta_by_location"].as<std::vector<double>>(),
            std::vector<double>{0.1});

  // Test missing mode
  node = YAML::Node();
  node[SpatialSettings::GRID_BASED_MODE]["population_raster"] = "pop.asc";
  EXPECT_THROW(YAML::convert<SpatialSettings>::decode(node, decoded_settings), std::runtime_error);

  // Test missing mode-specific node
  node = YAML::Node();
  node["mode"] = SpatialSettings::GRID_BASED_MODE;
  EXPECT_THROW(YAML::convert<SpatialSettings>::decode(node, decoded_settings), std::runtime_error);

  // Test unknown mode
  node = YAML::Node();
  node["mode"] = "unknown_mode";
  node["unknown_mode"]["data"] = 1;
  EXPECT_THROW(YAML::convert<SpatialSettings>::decode(node, decoded_settings), std::runtime_error);
}

// Test decoding functionality for GridBased settings
TEST(SpatialSettingsDecodeTest, DecodeGridBasedSettings) {
  YAML::Node node;
  node["population_raster"] = "test_pop.asc";
  node["administrative_boundaries"][0]["name"] = "district";
  node["administrative_boundaries"][0]["raster"] = "test_district.asc";
  node["administrative_boundaries"][1]["name"] = "region";
  node["administrative_boundaries"][1]["raster"] = "test_region.asc";
  node["p_treatment_under_5_raster"] = "test_treatment.asc";
  node["p_treatment_over_5_raster"] = "test_treatment.asc";
  node["beta_raster"] = "test_beta.asc";
  node["ecoclimatic_raster"] = "test_eco.asc";
  node["cell_size"] = 5.0;
  std::vector<std::vector<double>> age_dist = {
      {0.0378, 0.0378, 0.0378, 0.0378, 0.0282, 0.0282, 0.0282, 0.0282, 0.0282, 0.029, 0.029,
       0.029,  0.029,  0.029,  0.169,  0.134,  0.106,  0.066,  0.053,  0.035,  0.0}};
  node["age_distribution_by_location"] = age_dist;

  SpatialSettings::GridBased grid_settings;
  EXPECT_TRUE(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings));

  EXPECT_EQ(grid_settings.population_raster, "test_pop.asc");
  EXPECT_EQ(grid_settings.p_treatment_under_5_raster, "test_treatment.asc");
  EXPECT_EQ(grid_settings.p_treatment_over_5_raster, "test_treatment.asc");
  EXPECT_EQ(grid_settings.beta_raster, "test_beta.asc");
  EXPECT_EQ(grid_settings.ecoclimatic_raster, "test_eco.asc");
  EXPECT_EQ(grid_settings.cell_size, 5.0);

  ASSERT_EQ(grid_settings.administrative_boundaries.size(), 2);
  EXPECT_EQ(grid_settings.administrative_boundaries[0].name, "district");
  EXPECT_EQ(grid_settings.administrative_boundaries[0].raster, "test_district.asc");
  EXPECT_EQ(grid_settings.administrative_boundaries[1].name, "region");
  EXPECT_EQ(grid_settings.administrative_boundaries[1].raster, "test_region.asc");

  ASSERT_EQ(grid_settings.age_distribution_by_location.size(), 1);
  CompareVectors(grid_settings.age_distribution_by_location[0], age_dist[0], age_dist[0].size());
}

// Test decoding functionality for LocationBased settings
TEST(SpatialSettingsDecodeTest, DecodeLocationBasedSettings) {
  YAML::Node node;
  std::vector<std::vector<double>> loc_info_raw = {{1, 10.0, 20.0}, {2, 15.0, 25.0}};
  std::vector<std::vector<double>> age_dist = {
      {0.0378, 0.0378, 0.0378, 0.0378, 0.0282, 0.0282, 0.0282, 0.0282, 0.0282, 0.029, 0.029,
       0.029,  0.029,  0.029,  0.169,  0.134,  0.106,  0.066,  0.053,  0.035,  0.0}};
  std::vector<double> p_under5 = {0.5, 0.6};
  std::vector<double> p_over5 = {0.4, 0.5};
  std::vector<double> beta = {0.05, 0.06};
  std::vector<int> pop_size = {1000, 1200};  // Correct type is vector<int>

  node["location_info"] = loc_info_raw;
  node["age_distribution_by_location"] = age_dist;
  node["p_treatment_under_5_by_location"] = p_under5;
  node["p_treatment_over_5_by_location"] = p_over5;
  node["beta_by_location"] = beta;
  node["population_size_by_location"] = pop_size;

  SpatialSettings::LocationBased location_settings;
  EXPECT_TRUE(YAML::convert<SpatialSettings::LocationBased>::decode(node, location_settings));

  ASSERT_EQ(location_settings.locations.size(), 2);
  EXPECT_EQ(location_settings.locations[0].id, 1);
  EXPECT_FLOAT_EQ(location_settings.locations[0].coordinate.latitude, 10.0f);
  EXPECT_FLOAT_EQ(location_settings.locations[0].coordinate.longitude, 20.0f);
  EXPECT_EQ(location_settings.locations[1].id, 2);
  EXPECT_FLOAT_EQ(location_settings.locations[1].coordinate.latitude, 15.0f);
  EXPECT_FLOAT_EQ(location_settings.locations[1].coordinate.longitude, 25.0f);

  ASSERT_EQ(location_settings.age_distribution_by_location.size(), 1);
  CompareVectors(location_settings.age_distribution_by_location[0], age_dist[0],
                 age_dist[0].size());

  CompareVectors(location_settings.p_treatment_under_5_by_location, p_under5, p_under5.size());
  CompareVectors(location_settings.p_treatment_over_5_by_location, p_over5, p_over5.size());
  CompareVectors(location_settings.beta_by_location, beta, beta.size());
  // Need CompareVectors for int or direct comparison
  ASSERT_EQ(location_settings.population_size_by_location.size(), pop_size.size());
  for (size_t i = 0; i < pop_size.size(); ++i) {
    EXPECT_EQ(location_settings.population_size_by_location[i], pop_size[i]);
  }
}

// Test for decoding with missing fields in GridBased
TEST(SpatialSettingsDecodeTest, DecodeGridBasedSettingsMissingField) {
  SpatialSettings::GridBased grid_settings;
  YAML::Node node;

  // Test missing population_raster
  node = YAML::Node();
  node["administrative_boundaries"][0]["name"] = "district";
  node["administrative_boundaries"][0]["raster"] = "dist.asc";
  node["p_treatment_under_5_raster"] = "treat_u5.asc";
  node["p_treatment_over_5_raster"] = "treat_o5.asc";
  node["beta_raster"] = "beta.asc";
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  EXPECT_THROW(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings),
               std::runtime_error);

  // Test missing administrative_boundaries
  node = YAML::Node();
  node["population_raster"] = "pop.asc";
  // Missing administrative_boundaries
  node["p_treatment_under_5_raster"] = "treat_u5.asc";
  node["p_treatment_over_5_raster"] = "treat_o5.asc";
  node["beta_raster"] = "beta.asc";
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  EXPECT_THROW(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings),
               std::runtime_error);

  // ... Add tests for other missing fields (p_treatment_under_5_raster, etc.) ...
  // Test missing p_treatment_under_5_raster
  node = YAML::Node();
  node["population_raster"] = "pop.asc";
  node["administrative_boundaries"][0]["name"] = "district";
  node["administrative_boundaries"][0]["raster"] = "dist.asc";
  // node["p_treatment_under_5_raster"] = "treat_u5.asc"; // Missing
  node["p_treatment_over_5_raster"] = "treat_o5.asc";
  node["beta_raster"] = "beta.asc";
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  EXPECT_THROW(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings),
               std::runtime_error);

  // Test missing p_treatment_over_5_raster
  node = YAML::Node();
  node["population_raster"] = "pop.asc";
  node["administrative_boundaries"][0]["name"] = "district";
  node["administrative_boundaries"][0]["raster"] = "dist.asc";
  node["p_treatment_under_5_raster"] = "treat_u5.asc";
  // node["p_treatment_over_5_raster"] = "treat_o5.asc"; // Missing
  node["beta_raster"] = "beta.asc";
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  EXPECT_THROW(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings),
               std::runtime_error);

  // Test missing beta_raster
  node = YAML::Node();
  node["population_raster"] = "pop.asc";
  node["administrative_boundaries"][0]["name"] = "district";
  node["administrative_boundaries"][0]["raster"] = "dist.asc";
  node["p_treatment_under_5_raster"] = "treat_u5.asc";
  node["p_treatment_over_5_raster"] = "treat_o5.asc";
  // node["beta_raster"] = "beta.asc"; // Missing
  node["cell_size"] = 5.0;
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  EXPECT_THROW(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings),
               std::runtime_error);

  // Test missing cell_size
  node = YAML::Node();
  node["population_raster"] = "pop.asc";
  node["administrative_boundaries"][0]["name"] = "district";
  node["administrative_boundaries"][0]["raster"] = "dist.asc";
  node["p_treatment_under_5_raster"] = "treat_u5.asc";
  node["p_treatment_over_5_raster"] = "treat_o5.asc";
  node["beta_raster"] = "beta.asc";
  // node["cell_size"] = 5.0; // Missing
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  EXPECT_THROW(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings),
               std::runtime_error);

  // Test missing age_distribution_by_location
  node = YAML::Node();
  node["population_raster"] = "pop.asc";
  node["administrative_boundaries"][0]["name"] = "district";
  node["administrative_boundaries"][0]["raster"] = "dist.asc";
  node["p_treatment_under_5_raster"] = "treat_u5.asc";
  node["p_treatment_over_5_raster"] = "treat_o5.asc";
  node["beta_raster"] = "beta.asc";
  node["cell_size"] = 5.0;
  // node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}}; // Missing
  EXPECT_THROW(YAML::convert<SpatialSettings::GridBased>::decode(node, grid_settings),
               std::runtime_error);
}

// Test for decoding with missing fields in LocationBased
TEST(SpatialSettingsDecodeTest, DecodeLocationBasedSettingsMissingField) {
  SpatialSettings::LocationBased location_settings;
  YAML::Node node;

  // Test missing location_info
  node = YAML::Node();
  // node["location_info"] = std::vector<std::vector<double>>{{1, 10.0, 20.0}}; // Missing
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  node["p_treatment_under_5_by_location"] = std::vector<double>{0.5};
  node["p_treatment_over_5_by_location"] = std::vector<double>{0.4};
  node["beta_by_location"] = std::vector<double>{0.05};
  node["population_size_by_location"] = std::vector<int>{1000};
  EXPECT_THROW(YAML::convert<SpatialSettings::LocationBased>::decode(node, location_settings),
               std::runtime_error);

  // Test missing age_distribution_by_location
  node = YAML::Node();
  node["location_info"] = std::vector<std::vector<double>>{{1, 10.0, 20.0}};
  // node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}}; // Missing
  node["p_treatment_under_5_by_location"] = std::vector<double>{0.5};
  node["p_treatment_over_5_by_location"] = std::vector<double>{0.4};
  node["beta_by_location"] = std::vector<double>{0.05};
  node["population_size_by_location"] = std::vector<int>{1000};
  EXPECT_THROW(YAML::convert<SpatialSettings::LocationBased>::decode(node, location_settings),
               std::runtime_error);

  // ... Add tests for other missing fields ...
  // Test missing p_treatment_under_5_by_location
  node = YAML::Node();
  node["location_info"] = std::vector<std::vector<double>>{{1, 10.0, 20.0}};
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  // node["p_treatment_under_5_by_location"] = std::vector<double>{0.5}; // Missing
  node["p_treatment_over_5_by_location"] = std::vector<double>{0.4};
  node["beta_by_location"] = std::vector<double>{0.05};
  node["population_size_by_location"] = std::vector<int>{1000};
  EXPECT_THROW(YAML::convert<SpatialSettings::LocationBased>::decode(node, location_settings),
               std::runtime_error);

  // Test missing p_treatment_over_5_by_location
  node = YAML::Node();
  node["location_info"] = std::vector<std::vector<double>>{{1, 10.0, 20.0}};
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  node["p_treatment_under_5_by_location"] = std::vector<double>{0.5};
  // node["p_treatment_over_5_by_location"] = std::vector<double>{0.4}; // Missing
  node["beta_by_location"] = std::vector<double>{0.05};
  node["population_size_by_location"] = std::vector<int>{1000};
  EXPECT_THROW(YAML::convert<SpatialSettings::LocationBased>::decode(node, location_settings),
               std::runtime_error);

  // Test missing beta_by_location
  node = YAML::Node();
  node["location_info"] = std::vector<std::vector<double>>{{1, 10.0, 20.0}};
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  node["p_treatment_under_5_by_location"] = std::vector<double>{0.5};
  node["p_treatment_over_5_by_location"] = std::vector<double>{0.4};
  // node["beta_by_location"] = std::vector<double>{0.05}; // Missing
  node["population_size_by_location"] = std::vector<int>{1000};
  EXPECT_THROW(YAML::convert<SpatialSettings::LocationBased>::decode(node, location_settings),
               std::runtime_error);

  // Test missing population_size_by_location
  node = YAML::Node();
  node["location_info"] = std::vector<std::vector<double>>{{1, 10.0, 20.0}};
  node["age_distribution_by_location"] = std::vector<std::vector<double>>{{0.1}};
  node["p_treatment_under_5_by_location"] = std::vector<double>{0.5};
  node["p_treatment_over_5_by_location"] = std::vector<double>{0.4};
  node["beta_by_location"] = std::vector<double>{0.05};
  // node["population_size_by_location"] = std::vector<int>{1000}; // Missing
  EXPECT_THROW(YAML::convert<SpatialSettings::LocationBased>::decode(node, location_settings),
               std::runtime_error);
}

// Test for decoding SpatialSettings overall - redundant checks removed
// TEST_F(SpatialSettingsTest, DecodeSpatialSettingsMissingField) {
//   YAML::Node node;
//   node["mode"] = "grid_based";  // Only set mode, leaving other fields empty

//   SpatialSettings decoded_settings;
//   // This test is covered by DecodeSpatialSettingsModeAndNode
//   //EXPECT_THROW(YAML::convert<SpatialSettings>::decode(node, decoded_settings),
//   std::runtime_error);
// }
