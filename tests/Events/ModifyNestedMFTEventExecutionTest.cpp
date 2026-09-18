#include <gtest/gtest.h>

#include "Events/Population/ModifyNestedMFTEvent.h"
#include "Simulation/Model.h"
#include "Treatment/Strategies/NestedMFTMultiLocationStrategy.h"
#include "Treatment/Strategies/PublicPrivateStrategy.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ModifyNestedMFTEventExecutionTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node& config) {
      config["strategy_parameters"]["initial_strategy_id"] = 13;
    });
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

TEST_F(ModifyNestedMFTEventExecutionTest, ReplacesNestedMultiLocationPublicStrategy) {
  ASSERT_EQ(Model::get_treatment_strategy()->type, IStrategy::NestedMFTMultiLocation);
  auto* nested = dynamic_cast<NestedMFTMultiLocationStrategy*>(Model::get_treatment_strategy());
  ASSERT_NE(nested, nullptr);

  ModifyNestedMFTEvent event(0, 0);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());

  ASSERT_EQ(nested->strategy_list.size(), 2U);
  EXPECT_EQ(nested->strategy_list[0], Model::get_strategy_db()[0].get());
}

class ModifyNestedMFTEventVariantsTest : public ::testing::TestWithParam<int> {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node& config) {
      const int id = 11;
      config["strategy_parameters"]["initial_strategy_id"] = id;
    });
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

TEST_P(ModifyNestedMFTEventVariantsTest, ReplacesPublicStrategyForSupportedWrapper) {
  ASSERT_EQ(Model::get_treatment_strategy()->type, IStrategy::PublicPrivate);
  ModifyNestedMFTEvent event(0, 1);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());

  auto* wrapper = dynamic_cast<PublicPrivateStrategy*>(Model::get_treatment_strategy());
  ASSERT_NE(wrapper, nullptr);
  EXPECT_EQ(wrapper->get_public_strategy(), Model::get_strategy_db()[1].get());
}

INSTANTIATE_TEST_SUITE_P(SupportedWrappers, ModifyNestedMFTEventVariantsTest,
                         ::testing::Values(11));
