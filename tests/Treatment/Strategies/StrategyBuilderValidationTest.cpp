#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Simulation/Model.h"
#include "Treatment/Strategies/StrategyBuilder.h"
#include "Treatment/Strategies/PublicPrivateStrategy.h"
#include "Treatment/Strategies/PublicPrivateMultiLocationStrategy.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class StrategyBuilderValidationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    Model::get_instance()->initialize();
  }
  void TearDown() override { test_fixtures::cleanup_test_files(); }
};

TEST_F(StrategyBuilderValidationTest, RejectsInvalidPublicPrivateAndAgeBasedConfigurations) {
  auto public_private = YAML::Load(R"(
    name: public-private
    type: PublicPrivate
    public_strategy_id: 0
    private_strategy_id: 1
    start_public_share: 0.3
    peak_public_share: 0.7
    peak_after: 30
  )");
  public_private.remove("public_strategy_id");
  EXPECT_THROW(StrategyBuilder::build(public_private, 2), std::invalid_argument);
  public_private["public_strategy_id"] = 0;
  public_private["private_strategy_id"] = 0;
  EXPECT_THROW(StrategyBuilder::build(public_private, 2), std::invalid_argument);
  public_private["private_strategy_id"] = 9999;
  EXPECT_THROW(StrategyBuilder::build(public_private, 2), std::invalid_argument);
  public_private["private_strategy_id"] = 1;
  public_private.remove("start_public_share");
  EXPECT_THROW(StrategyBuilder::build(public_private, 2), std::invalid_argument);
  public_private["start_public_share"] = ".nan";
  EXPECT_THROW(StrategyBuilder::build(public_private, 2), std::invalid_argument);

  auto age_based = YAML::Load(R"(
    name: age-based
    type: MFTAgeBased
    therapy_ids: [6, 8]
    distribution: [0.5, 0.5]
    age_boundaries: []
  )");
  EXPECT_THROW(StrategyBuilder::build(age_based, 2), std::invalid_argument);
}

TEST_F(StrategyBuilderValidationTest, ValidatesPublicPrivateStrategyFields) {
  auto node = YAML::Load("name: public-private\ntype: PublicPrivate\npublic_strategy_id: 0\nprivate_strategy_id: 1\nstart_public_share: 0.3\npeak_public_share: 0.7\npeak_after: 30");
  if (Model::get_strategy_db().size() > 1) {
    auto strategy = StrategyBuilder::build(node, 2);
    auto* result = dynamic_cast<PublicPrivateStrategy*>(strategy.get());
    ASSERT_NE(result, nullptr);
    EXPECT_DOUBLE_EQ(result->start_public_share, 0.3);
    EXPECT_DOUBLE_EQ(result->peak_public_share, 0.7);
  }
  node["start_public_share"] = 1.1;
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
}

TEST_F(StrategyBuilderValidationTest, ValidatesPublicPrivateMultiLocationFields) {
  auto node = YAML::Load("name: public-private-multi\ntype: PublicPrivateMultiLocation\npublic_strategy_id: 0\nprivate_strategy_id: 1\npeak_after: 30");
  YAML::Node starts, peaks;
  for (int i = 0; i < Model::get_config()->number_of_locations(); ++i) {
    starts.push_back(0.3);
    peaks.push_back(0.7);
  }
  node["start_public_share_by_location"] = starts;
  node["peak_public_share_by_location"] = peaks;
  ASSERT_NE(dynamic_cast<PublicPrivateMultiLocationStrategy*>(StrategyBuilder::build(node, 2).get()), nullptr);
  node["start_public_share_by_location"][0] = 1.1;
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
  node["start_public_share_by_location"][0] = 0.3;
  node.remove("peak_after");
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
  node["peak_after"] = 30;
  node["public_strategy_id"] = 2;
  node["private_strategy_id"] = 2;
  EXPECT_THROW(StrategyBuilder::build(node, 3), std::invalid_argument);
  node["public_strategy_id"] = 0;
  node["private_strategy_id"] = 1;
  node["peak_after"] = -1;
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
  node["peak_after"] = 30;
  node["start_public_share_by_location"] = YAML::Load("[0.3, 0.3]");
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
  node.remove("start_public_share_by_location");
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
  node["start_public_share_by_location"] = starts;
  node.remove("private_strategy_id");
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
  node["private_strategy_id"] = 9999;
  EXPECT_THROW(StrategyBuilder::build(node, 2), std::invalid_argument);
}
