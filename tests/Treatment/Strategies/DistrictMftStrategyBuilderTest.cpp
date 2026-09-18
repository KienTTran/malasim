#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <type_traits>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

#include "Treatment/Strategies/DistrictMftStrategy.h"
#include "Treatment/Strategies/IStrategy.h"
#include "Treatment/Strategies/StrategyBuilder.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class DistrictMftStrategyBuilderTest : public ::testing::Test {
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

  static constexpr int k_district_count = 3;

  YAML::Node create_district_mft_node(const std::string& definitions_yaml) {
    YAML::Node node;
    node["name"] = "Test District MFT";
    node["type"] = "DistrictMFT";
    node["definitions"] = YAML::Load(definitions_yaml);
    return node;
  }

  YAML::Node create_full_coverage_district_mft_node() {
    return create_district_mft_node(R"(
      0:
        district_ids: [1, 2]
        therapy_ids: [6, 8, 2, 0, 12, 14, 5]
        distribution: [0.765, 0.085, 0.00225, 0.06, 0.00075, 0.027, 0.06]
      1:
        district_ids: [3]
        therapy_ids: [6, 8]
        distribution: [0.5, 0.5]
    )");
  }
};

TEST_F(DistrictMftStrategyBuilderTest, FixtureProvidesThreeDistricts) {
  const auto* boundary = Model::get_spatial_data()->get_boundary("district");
  ASSERT_NE(boundary, nullptr);
  EXPECT_EQ(boundary->min_unit_id, 1);
  EXPECT_EQ(boundary->max_unit_id, k_district_count);
  EXPECT_EQ(boundary->unit_count, k_district_count);
}

TEST_F(DistrictMftStrategyBuilderTest, BuildDistrictMftStrategy) {
  auto strategy = StrategyBuilder::build(create_full_coverage_district_mft_node(), 11);
  ASSERT_NE(strategy, nullptr);
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::DistrictMft);
  EXPECT_NE(dynamic_cast<DistrictMftStrategy*>(strategy.get()), nullptr);
  EXPECT_EQ(strategy->id, 11);
  EXPECT_EQ(strategy->name, "Test District MFT");
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftAcceptsDistributionThatSumsToOneInDecimal) {
  auto node = create_district_mft_node(R"(
    0:
      district_ids: [1, 2, 3]
      therapy_ids: [6, 8, 2, 0, 12, 14, 5]
      distribution: [0.765, 0.085, 0.0045, 0.063, 0.015, 0.006, 0.0615]
  )");
  EXPECT_NO_THROW({ EXPECT_EQ(StrategyBuilder::build(node, 12)->get_type(), IStrategy::StrategyType::DistrictMft); });
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftRejectsDistributionSumMismatch) {
  auto node = create_district_mft_node("0: {district_ids: [1, 2, 3], therapy_ids: [6, 8], distribution: [0.5, 0.4]}");
  EXPECT_THROW(StrategyBuilder::build(node, 13), std::invalid_argument);
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftRejectsIncompleteDistrictCoverage) {
  auto node = create_district_mft_node("0: {district_ids: [1, 2], therapy_ids: [6, 8], distribution: [0.5, 0.5]}");
  EXPECT_THROW(StrategyBuilder::build(node, 14), std::invalid_argument);
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftRejectsDuplicateDistrictAssignment) {
  auto node = create_district_mft_node(R"(0: {district_ids: [1, 2, 3], therapy_ids: [6, 8], distribution: [0.5, 0.5]}
1: {district_ids: [2], therapy_ids: [6, 8], distribution: [0.5, 0.5]})");
  EXPECT_THROW(StrategyBuilder::build(node, 15), std::invalid_argument);
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftRejectsOutOfRangeDistrictId) {
  auto node = create_district_mft_node("0: {district_ids: [1, 2, 3, 4], therapy_ids: [6, 8], distribution: [0.5, 0.5]}");
  EXPECT_THROW(StrategyBuilder::build(node, 16), std::invalid_argument);
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftRejectsTherapyIdEqualToDatabaseSize) {
  const auto therapy_count = static_cast<int>(Model::get_therapy_db().size());
  ASSERT_GT(therapy_count, 0);
  auto node = create_district_mft_node("0: {district_ids: [1, 2, 3], therapy_ids: [" + std::to_string(therapy_count) + ", 8], distribution: [0.5, 0.5]}");
  EXPECT_THROW(StrategyBuilder::build(node, 17), std::invalid_argument);
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftRejectsMismatchedTherapyAndDistributionSizes) {
  auto node = create_district_mft_node("0: {district_ids: [1, 2, 3], therapy_ids: [6, 8, 2], distribution: [0.5, 0.5]}");
  EXPECT_THROW(StrategyBuilder::build(node, 18), std::invalid_argument);
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftAcceptsZeroWeightedTherapy) {
  auto node = create_district_mft_node("0: {district_ids: [1, 2, 3], therapy_ids: [6, 8, 2], distribution: [0.5, 0.5, 0.0]}");
  EXPECT_NO_THROW(StrategyBuilder::build(node, 19));
}

TEST_F(DistrictMftStrategyBuilderTest, DistrictMftStoresSharesAsDouble) {
  static_assert(std::is_same_v<decltype(DistrictMftStrategy::MftStrategy::percentages)::value_type, double>);
  SUCCEED();
}
