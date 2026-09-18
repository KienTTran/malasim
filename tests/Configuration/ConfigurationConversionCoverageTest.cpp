#include <gtest/gtest.h>

#include <string>

#include "Configuration/MosquitoParameters.h"
#include "Configuration/SpatialSettings/SpatialSettings.h"
#include "Configuration/TransmissionSettings.h"

TEST(ConfigurationConversionCoverageTest, CoversMosquitoParameterConversionsAndValidation) {
  MosquitoParameters::GridBased grid;
  grid.set_interrupted_feeding_rate_raster("feeding.asc");
  grid.set_prmc_size_raster("prmc.asc");
  auto grid_node = YAML::convert<MosquitoParameters::GridBased>::encode(grid);
  auto decoded_grid = grid_node.as<MosquitoParameters::GridBased>();
  EXPECT_EQ(decoded_grid.get_interrupted_feeding_rate_raster(), "feeding.asc");
  EXPECT_EQ(decoded_grid.get_prmc_size_raster(), "prmc.asc");
  EXPECT_THROW(YAML::convert<MosquitoParameters::GridBased>::decode(YAML::Node(), decoded_grid),
               std::runtime_error);

  MosquitoParameters::LocationBased location;
  location.set_interrupted_feeding_rate({0.1, 0.2});
  location.set_prmc_size({10, 20});
  auto location_node = YAML::convert<MosquitoParameters::LocationBased>::encode(location);
  auto decoded_location = location_node.as<MosquitoParameters::LocationBased>();
  EXPECT_EQ(decoded_location.get_prmc_size(), std::vector<int>({10, 20}));
  EXPECT_THROW(
      YAML::convert<MosquitoParameters::LocationBased>::decode(YAML::Node(), decoded_location),
      std::runtime_error);

  MosquitoParameters::MosquitoConfig grid_config;
  grid_config.set_mode("grid_based");
  grid_config.set_grid_based(grid);
  auto decoded_grid_config =
      YAML::convert<MosquitoParameters::MosquitoConfig>::encode(grid_config)
          .as<MosquitoParameters::MosquitoConfig>();
  EXPECT_EQ(decoded_grid_config.get_mode(), "grid_based");

  MosquitoParameters::MosquitoConfig location_config;
  location_config.set_mode("location_based");
  location_config.set_location_based(location);
  auto decoded_location_config =
      YAML::convert<MosquitoParameters::MosquitoConfig>::encode(location_config)
          .as<MosquitoParameters::MosquitoConfig>();
  EXPECT_EQ(decoded_location_config.get_mode(), "location_based");
  EXPECT_THROW(grid_config.set_mode("invalid"), std::invalid_argument);
  EXPECT_THROW(YAML::convert<MosquitoParameters::MosquitoConfig>::decode(
                   YAML::Load("mode: invalid"), grid_config),
               std::invalid_argument);

  MosquitoParameters parameters;
  parameters.set_mosquito_config(grid_config);
  parameters.set_within_host_induced_free_recombination(false);
  parameters.set_record_recombination_events(true);
  auto parameters_node = YAML::convert<MosquitoParameters>::encode(parameters);
  MosquitoParameters decoded_parameters;
  ASSERT_TRUE(YAML::convert<MosquitoParameters>::decode(parameters_node, decoded_parameters));
  EXPECT_FALSE(decoded_parameters.get_within_host_induced_free_recombination());
  EXPECT_TRUE(decoded_parameters.get_record_recombination_events());
  auto no_optional_record = parameters_node;
  no_optional_record.remove("record_recombination_events");
  MosquitoParameters decoded_without_optional_record;
  ASSERT_TRUE(YAML::convert<MosquitoParameters>::decode(no_optional_record,
                                                        decoded_without_optional_record));
  EXPECT_FALSE(decoded_without_optional_record.get_record_recombination_events());
  EXPECT_THROW(YAML::convert<MosquitoParameters>::decode(YAML::Node(), parameters),
               std::runtime_error);
}

TEST(ConfigurationConversionCoverageTest, CoversTransmissionValidationAndYamlConversion) {
  TransmissionSettings settings;
  settings.set_transmission_parameter(0.4);
  settings.set_p_infection_from_an_infectious_bite(0.8);
  EXPECT_DOUBLE_EQ(settings.get_transmission_parameter(), 0.4);
  EXPECT_DOUBLE_EQ(settings.get_p_infection_from_an_infectious_bite(), 0.8);
  EXPECT_THROW(settings.set_transmission_parameter(-0.1), std::invalid_argument);
  EXPECT_THROW(settings.set_p_infection_from_an_infectious_bite(-0.1), std::invalid_argument);
  EXPECT_THROW(settings.set_p_infection_from_an_infectious_bite(1.1), std::invalid_argument);

  const auto node = YAML::convert<TransmissionSettings>::encode(settings);
  TransmissionSettings decoded;
  ASSERT_TRUE(YAML::convert<TransmissionSettings>::decode(node, decoded));
  EXPECT_DOUBLE_EQ(decoded.get_transmission_parameter(), 0.4);
  EXPECT_DOUBLE_EQ(decoded.get_p_infection_from_an_infectious_bite(), 0.8);
  EXPECT_THROW(YAML::convert<TransmissionSettings>::decode(YAML::Load("{}"), settings),
               std::runtime_error);
}

TEST(ConfigurationConversionCoverageTest, CoversSpatialSettingsConversionsAndModes) {
  Spatial::Location first;
  first.id = 7;
  first.coordinate = Spatial::Coordinate(10.5F, 20.5F);
  Spatial::Location second;
  second.id = 8;
  second.coordinate = Spatial::Coordinate(11.5F, 21.5F);

  const auto location_node = YAML::convert<Spatial::Location>::encode(first);
  const auto decoded_location = location_node.as<Spatial::Location>();
  EXPECT_EQ(decoded_location.id, 7);
  EXPECT_FLOAT_EQ(decoded_location.coordinate.latitude, 10.5F);
  EXPECT_THROW(YAML::convert<Spatial::Location>::decode(YAML::Load("[1, 2]"), first),
               std::runtime_error);

  SpatialSettings::GridBased grid;
  grid.population_raster = "population.asc";
  grid.p_treatment_under_5_raster = "under.asc";
  grid.p_treatment_over_5_raster = "over.asc";
  grid.beta_raster = "beta.asc";
  grid.ecoclimatic_raster = "eco.asc";
  grid.cell_size = 100.0;
  grid.age_distribution_by_location = {{0.5, 0.5}};
  grid.administrative_boundaries = {{"district", "district.asc"}};
  const auto grid_settings_node = YAML::convert<SpatialSettings::GridBased>::encode(grid);
  const auto decoded_grid = grid_settings_node.as<SpatialSettings::GridBased>();
  EXPECT_EQ(decoded_grid.administrative_boundaries.size(), 1U);
  EXPECT_EQ(decoded_grid.ecoclimatic_raster, "eco.asc");
  auto missing_grid = grid_settings_node;
  missing_grid.remove("beta_raster");
  EXPECT_THROW(missing_grid.as<SpatialSettings::GridBased>(), std::runtime_error);

  SpatialSettings::LocationBased location;
  location.locations = {first, second};
  location.age_distribution_by_location = {{0.5, 0.5}, {0.4, 0.6}};
  location.p_treatment_under_5_by_location = {0.1, 0.2};
  location.p_treatment_over_5_by_location = {0.3, 0.4};
  location.beta_by_location = {1.0, 2.0};
  location.population_size_by_location = {100, 200};
  const auto location_settings_node = YAML::convert<SpatialSettings::LocationBased>::encode(location);
  const auto decoded_location_settings =
      location_settings_node.as<SpatialSettings::LocationBased>();
  EXPECT_EQ(decoded_location_settings.locations.size(), 2U);
  auto missing_location = location_settings_node;
  missing_location.remove("beta_by_location");
  EXPECT_THROW(missing_location.as<SpatialSettings::LocationBased>(), std::runtime_error);

  SpatialSettings spatial;
  spatial.set_mode(SpatialSettings::GRID_BASED_MODE);
  spatial.set_node(grid_settings_node);
  const auto encoded_spatial = YAML::convert<SpatialSettings>::encode(spatial);
  SpatialSettings decoded_spatial;
  ASSERT_TRUE(YAML::convert<SpatialSettings>::decode(encoded_spatial, decoded_spatial));
  EXPECT_EQ(decoded_spatial.get_mode(), SpatialSettings::GRID_BASED_MODE);
  EXPECT_THROW(YAML::convert<SpatialSettings>::decode(YAML::Load("mode: unknown"), spatial),
               std::runtime_error);
  EXPECT_THROW(YAML::convert<SpatialSettings>::decode(YAML::Load("mode: grid_based"), spatial),
               std::runtime_error);
}
