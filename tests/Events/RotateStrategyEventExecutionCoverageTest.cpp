#include <gtest/gtest.h>

#include "Events/Population/RotateStrategyEvent.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class RotateStrategyEventExecutionCoverageTest : public ::testing::Test {
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

TEST_F(RotateStrategyEventExecutionCoverageTest, SwitchesStrategyAndSchedulesNextRotation) {
  RotateStrategyEvent event(0, 1, 0, 0);
  event.set_executable(true);

  ASSERT_NO_THROW(event.execute());
  EXPECT_FALSE(event.is_executable());
  EXPECT_EQ(Model::get_treatment_strategy()->id, 0);
  EXPECT_FALSE(Model::get_scheduler()->get_world_events().get_events().empty());
}
