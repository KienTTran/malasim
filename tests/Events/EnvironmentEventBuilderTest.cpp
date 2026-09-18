#include <gtest/gtest.h>

#include "Configuration/Config.h"
#include "Events/Environment/EnvironmentEventBuilder.h"
#include "Events/Environment/UpdateEcozoneEvent.hxx"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class EnvironmentEventBuilderTest : public ::testing::Test {
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

TEST_F(EnvironmentEventBuilderTest, BuildsEcozoneUpdatesAndDispatchesByName) {
  const auto event = EnvironmentEventBuilder::build_update_ecozone_event(
      YAML::Load("- day: 2024/01/03\n  from: 1\n  to: 2"), Model::get_config());
  ASSERT_EQ(event.size(), 1);
  EXPECT_EQ(event[0]->name(), UpdateEcozoneEvent::EVENT_NAME);

  const auto dispatched = EnvironmentEventBuilder::build(YAML::Load(
      "name: update_ecozone\ninfo:\n  - day: 2024/01/03\n    from: 1\n    to: 2"));
  ASSERT_EQ(dispatched.size(), 1);
  EXPECT_EQ(dispatched[0]->name(), UpdateEcozoneEvent::EVENT_NAME);

  EXPECT_TRUE(EnvironmentEventBuilder::build(YAML::Load("name: unknown")).empty());
}

TEST_F(EnvironmentEventBuilderTest, RejectsNegativeEcozoneIds) {
  EXPECT_THROW(EnvironmentEventBuilder::build_update_ecozone_event(
                   YAML::Load("- day: 2024/01/03\n  from: -1\n  to: 2"), Model::get_config()),
               std::invalid_argument);
  EXPECT_THROW(EnvironmentEventBuilder::build_update_ecozone_event(
                   YAML::Load("- day: 2024/01/03\n  from: 1\n  to: -1"), Model::get_config()),
               std::invalid_argument);
}
