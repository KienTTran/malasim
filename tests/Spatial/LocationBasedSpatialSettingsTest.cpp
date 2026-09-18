#include <gtest/gtest.h>

#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class LocationBasedSpatialSettingsTest : public ::testing::Test {
protected:
  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(LocationBasedSpatialSettingsTest, InitializesLocationBasedProcessor) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["spatial_settings"]["mode"] = "location_based";
  });

  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());

  auto *settings = &Model::get_config()->get_spatial_settings();
  EXPECT_EQ(settings->get_mode(), "location_based");
  EXPECT_EQ(settings->get_number_of_locations(), 1);
  ASSERT_EQ(settings->location_db().size(), 1);
  EXPECT_EQ(settings->location_db()[0].population_size, 50000);
  EXPECT_DOUBLE_EQ(settings->location_db()[0].beta, 0.054);
  EXPECT_NE(settings->get_distance_provider(), nullptr);
  EXPECT_NE(settings->spatial_data(), nullptr);
}
