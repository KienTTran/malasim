#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "apps/efficacy_estimator/DxGCli.h"

namespace {

utils::DxGAppInput make_valid_ee_input() {
  utils::DxGAppInput input;
  input.is_ee_calibration = true;
  input.genotypes = {"WT"};
  input.half_life = {2.0};
  input.k_max = {0.8};
  input.ec50 = {4.0};
  input.slope = {1.2};
  input.dosing_days = {0};
  return input;
}

TEST(DxGCliValidationTest, AcceptsNonEeModesWithoutEeParameters) {
  utils::DxGAppInput input;

  EXPECT_TRUE(utils::cli::validate<utils::DxGCli>(input));
}

TEST(DxGCliValidationTest, AcceptsValidEeInputAndDefaultsAbsorption) {
  auto input = make_valid_ee_input();

  ASSERT_TRUE(utils::cli::validate<utils::DxGCli>(input));
  EXPECT_EQ(input.number_of_drugs_in_combination, 1);
  EXPECT_EQ(input.mean_drug_absorption, (std::vector<double>{1.0}));
}

TEST(DxGCliValidationTest, PreservesExplicitAbsorption) {
  auto input = make_valid_ee_input();
  input.mean_drug_absorption = {0.5};

  ASSERT_TRUE(utils::cli::validate<utils::DxGCli>(input));
  EXPECT_EQ(input.mean_drug_absorption, (std::vector<double>{0.5}));
}

TEST(DxGCliValidationTest, RejectsMoreThanFiveDrugs) {
  auto input = make_valid_ee_input();
  input.half_life = std::vector<double>(6, 2.0);

  EXPECT_FALSE(utils::cli::validate<utils::DxGCli>(input));
}

class DxGListLengthValidationTest
    : public ::testing::TestWithParam<std::function<void(utils::DxGAppInput &)>> {};

TEST_P(DxGListLengthValidationTest, RejectsMismatchedDrugParameterList) {
  auto input = make_valid_ee_input();
  GetParam()(input);

  EXPECT_FALSE(utils::cli::validate<utils::DxGCli>(input));
}

INSTANTIATE_TEST_SUITE_P(
    DrugParameterLists,
    DxGListLengthValidationTest,
    ::testing::Values(
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.k_max.clear(); }},
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.ec50.clear(); }},
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.slope.clear(); }},
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.dosing_days.clear(); }}));

class DxGValueValidationTest
    : public ::testing::TestWithParam<std::function<void(utils::DxGAppInput &)>> {};

TEST_P(DxGValueValidationTest, RejectsInvalidDrugParameterValue) {
  auto input = make_valid_ee_input();
  GetParam()(input);

  EXPECT_FALSE(utils::cli::validate<utils::DxGCli>(input));
}

INSTANTIATE_TEST_SUITE_P(
    DrugParameterValues,
    DxGValueValidationTest,
    ::testing::Values(
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.k_max = {1.0}; }},
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.k_max = {-0.1}; }},
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.ec50 = {-0.1}; }},
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.slope = {-0.1}; }},
        std::function<void(utils::DxGAppInput &)>{[](auto &input) { input.dosing_days = {-1}; }}));

TEST(DxGCliValidationTest, RejectsMismatchedAbsorptionList) {
  auto input = make_valid_ee_input();
  input.mean_drug_absorption = {0.5, 0.6};

  EXPECT_FALSE(utils::cli::validate<utils::DxGCli>(input));
}

TEST(DxGCliValidationTest, RejectsMultipleEeGenotypes) {
  auto input = make_valid_ee_input();
  input.genotypes = {"WT", "KEL1"};

  EXPECT_FALSE(utils::cli::validate<utils::DxGCli>(input));
}

static_assert(utils::cli::CliApplication<utils::DxGCli>);

}  // namespace
