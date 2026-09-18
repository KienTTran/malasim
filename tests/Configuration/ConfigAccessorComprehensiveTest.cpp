#include <gtest/gtest.h>

#include "Configuration/Config.h"

TEST(ConfigAccessorComprehensiveTest, ExposesConfigurationSectionsAndDerivedValues) {
  Config config;

  EXPECT_EQ(config.number_of_locations(), 0U);
  EXPECT_EQ(config.number_of_age_classes(), -1);
  EXPECT_EQ(config.number_of_tracking_days(), 11);
  EXPECT_TRUE(config.age_structure().empty());
  EXPECT_TRUE(config.location_db().empty());

  EXPECT_NE(&config.get_model_settings(), nullptr);
  EXPECT_NE(&config.get_simulation_timeframe(), nullptr);
  EXPECT_NE(&config.get_transmission_settings(), nullptr);
  EXPECT_NE(&config.get_population_demographic(), nullptr);
  EXPECT_NE(&config.get_epidemiological_parameters(), nullptr);
  EXPECT_NE(&config.get_parasite_parameters(), nullptr);
  EXPECT_NE(&config.get_spatial_settings(), nullptr);
  EXPECT_NE(&config.get_seasonality_settings(), nullptr);
  EXPECT_NE(&config.get_movement_settings(), nullptr);
  EXPECT_NE(&config.get_immune_system_parameters(), nullptr);
  EXPECT_NE(&config.get_version6_pfpr_incidence_calibrations(), nullptr);
  EXPECT_FALSE(config.has_version6_pfpr_incidence_calibrations());
  EXPECT_NE(&config.get_genotype_parameters(), nullptr);
  EXPECT_NE(&config.get_drug_parameters(), nullptr);
  EXPECT_NE(&config.get_therapy_parameters(), nullptr);
  EXPECT_NE(&config.get_strategy_parameters(), nullptr);
  EXPECT_NE(&config.get_mosquito_parameters(), nullptr);
  EXPECT_NE(&config.get_population_events(), nullptr);
  EXPECT_NE(&config.get_rapt_settings(), nullptr);
}

TEST(ConfigAccessorComprehensiveTest, MutableAccessorsUpdateStoredValues) {
  Config config;
  config.get_spatial_settings().set_number_of_locations(3);
  config.get_population_demographic().set_age_structure({7, 9});
  config.get_epidemiological_parameters().set_number_of_tracking_days(14);
  Spatial::Location location;
  location.id = 1;
  location.coordinate = Spatial::Coordinate{1.0F, 2.0F};
  config.location_db().push_back(location);

  EXPECT_EQ(config.number_of_locations(), 3U);
  EXPECT_EQ(config.number_of_age_classes(), 2);
  EXPECT_EQ(config.number_of_tracking_days(), 14);
  EXPECT_EQ(config.age_structure(), (std::vector<int>{7, 9}));
  ASSERT_EQ(config.location_db().size(), 1U);
  EXPECT_EQ(config.location_db().front().id, 1);
}

TEST(ConfigAccessorComprehensiveTest, ConstAccessorsAndStaticDerivedValue) {
  const Config config;
  EXPECT_NE(&config.get_model_settings(), nullptr);
  EXPECT_NE(&config.get_simulation_timeframe(), nullptr);
  EXPECT_NE(&config.get_transmission_settings(), nullptr);
  EXPECT_NE(&config.get_population_demographic(), nullptr);
  EXPECT_NE(&config.get_epidemiological_parameters(), nullptr);
  EXPECT_NE(&config.get_parasite_parameters(), nullptr);
  EXPECT_NE(&config.get_spatial_settings(), nullptr);
  EXPECT_NE(&config.get_seasonality_settings(), nullptr);
  EXPECT_NE(&config.get_movement_settings(), nullptr);
  EXPECT_NE(&config.get_immune_system_parameters(), nullptr);
  EXPECT_NE(&config.get_genotype_parameters(), nullptr);
  EXPECT_NE(&config.get_drug_parameters(), nullptr);
  EXPECT_NE(&config.get_therapy_parameters(), nullptr);
  EXPECT_NE(&config.get_strategy_parameters(), nullptr);
  EXPECT_NE(&config.get_mosquito_parameters(), nullptr);
  EXPECT_NE(&config.get_population_events(), nullptr);
  EXPECT_NE(&config.get_rapt_settings(), nullptr);
}

TEST(ConfigAccessorComprehensiveTest, ReloadUsesThePreviouslySelectedConfigurationPath) {
  Config config;
  EXPECT_FALSE(config.load("missing-config-for-reload-test.yml"));
  EXPECT_NO_THROW(config.reload());
}
