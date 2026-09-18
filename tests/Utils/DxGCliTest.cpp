#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "apps/efficacy_estimator/DxGCli.h"

namespace {

class DxGCliTest : public ::testing::Test {
protected:
  static constexpr const char* kTempInput = "dxg_cli_input.yml";

  void SetUp() override {
    std::ofstream output(kTempInput);
    output << "# temp\n";
  }

  void TearDown() override { std::remove(kTempInput); }
};

TEST_F(DxGCliTest, ParsesDefaultValuesWithoutModeMarker) {
  const char* argv[] = {"DxGGenerator"};
  const auto input = utils::cli::parse<utils::DxGCli>(1, const_cast<char**>(argv));

  EXPECT_EQ(input.input_file, "input.yml");
  EXPECT_TRUE(input.output_file.empty());
  EXPECT_TRUE(input.therapies.empty());
  EXPECT_TRUE(input.therapy_list.empty());
  EXPECT_TRUE(input.genotypes.empty());
  EXPECT_FALSE(input.is_crt_calibration);
  EXPECT_DOUBLE_EQ(input.as_iov, -1.0);
  EXPECT_DOUBLE_EQ(input.as_iiv, -1.0);
  EXPECT_DOUBLE_EQ(input.as_ec50, -1.0);
  EXPECT_FALSE(input.is_ee_calibration);
  EXPECT_EQ(input.number_of_drugs_in_combination, 1);
  EXPECT_FALSE(input.is_art);
  EXPECT_TRUE(input.dosing_days.empty());
  EXPECT_TRUE(input.half_life.empty());
  EXPECT_TRUE(input.k_max.empty());
  EXPECT_TRUE(input.ec50.empty());
  EXPECT_TRUE(input.slope.empty());
  EXPECT_TRUE(input.mean_drug_absorption.empty());
  EXPECT_EQ(input.population_size, 10000);
  EXPECT_FALSE(input.is_print_immunity_level);
  EXPECT_FALSE(input.is_old_format);
  EXPECT_FALSE(input.is_recurrence_test);
}

TEST_F(DxGCliTest, ParsesEveryOption) {
  const char* argv[] = {"DxGGenerator",
                        "-i",
                        kTempInput,
                        "-o",
                        "result.csv",
                        "-t",
                        "1",
                        "2",
                        "--tl",
                        "3",
                        "5",
                        "-g",
                        "WT",
                        "KEL1",
                        "--cc",
                        "--iov",
                        "0.1",
                        "--iiv",
                        "0.2",
                        "--as_ec50",
                        "3.0",
                        "--ee",
                        "--nd",
                        "2",
                        "--art",
                        "--dose",
                        "0",
                        "1",
                        "--halflife",
                        "2.0",
                        "3.0",
                        "--kmax",
                        "0.8",
                        "0.9",
                        "--EC50",
                        "4.0",
                        "5.0",
                        "--slope",
                        "1.2",
                        "1.3",
                        "--mda",
                        "0.5",
                        "0.6",
                        "--popsize",
                        "50",
                        "--pil",
                        "--old_format",
                        "--recurrence-test"};
  const auto input =
      utils::cli::parse<utils::DxGCli>(static_cast<int>(std::size(argv)), const_cast<char**>(argv));

  EXPECT_EQ(input.input_file, kTempInput);
  EXPECT_EQ(input.output_file, "result.csv");
  EXPECT_EQ(input.therapies, (std::vector<int>{1, 2}));
  EXPECT_EQ(input.therapy_list, (std::vector<int>{3, 5}));
  EXPECT_EQ(input.genotypes, (std::vector<std::string>{"WT", "KEL1"}));
  EXPECT_TRUE(input.is_crt_calibration);
  EXPECT_DOUBLE_EQ(input.as_iov, 0.1);
  EXPECT_DOUBLE_EQ(input.as_iiv, 0.2);
  EXPECT_DOUBLE_EQ(input.as_ec50, 3.0);
  EXPECT_TRUE(input.is_ee_calibration);
  EXPECT_EQ(input.number_of_drugs_in_combination, 2);
  EXPECT_TRUE(input.is_art);
  EXPECT_EQ(input.dosing_days, (std::vector<int>{0, 1}));
  EXPECT_EQ(input.half_life, (std::vector<double>{2.0, 3.0}));
  EXPECT_EQ(input.k_max, (std::vector<double>{0.8, 0.9}));
  EXPECT_EQ(input.ec50, (std::vector<double>{4.0, 5.0}));
  EXPECT_EQ(input.slope, (std::vector<double>{1.2, 1.3}));
  EXPECT_EQ(input.mean_drug_absorption, (std::vector<double>{0.5, 0.6}));
  EXPECT_EQ(input.population_size, 50);
  EXPECT_TRUE(input.is_print_immunity_level);
  EXPECT_TRUE(input.is_old_format);
  EXPECT_TRUE(input.is_recurrence_test);
}

class DxGLegacyMarkerTest : public DxGCliTest, public ::testing::WithParamInterface<const char*> {};

TEST_P(DxGLegacyMarkerTest, AcceptsLegacyModeMarker) {
  const char* argv[] = {"DxGGenerator", GetParam(), "-i", kTempInput};
  const auto input = utils::cli::parse<utils::DxGCli>(4, const_cast<char**>(argv));

  EXPECT_EQ(input.input_file, kTempInput);
}

INSTANTIATE_TEST_SUITE_P(LegacyMarkers,
                         DxGLegacyMarkerTest,
                         ::testing::Values("--DxG", "DxG=1", "DxG=true"));

TEST_F(DxGCliTest, RejectsMissingInputFile) {
  const char* argv[] = {"DxGGenerator", "-i", "definitely_missing_dxg_input.yml"};

  EXPECT_THROW(
      {
        const auto input = utils::cli::parse<utils::DxGCli>(3, const_cast<char**>(argv));
        static_cast<void>(input);
      },
      CLI::ParseError);
}

TEST_F(DxGCliTest, RejectsUnknownOption) {
  const char* argv[] = {"DxGGenerator", "--not-a-dxg-option"};

  EXPECT_THROW(
      {
        const auto input = utils::cli::parse<utils::DxGCli>(2, const_cast<char**>(argv));
        static_cast<void>(input);
      },
      CLI::ParseError);
}

TEST_F(DxGCliTest, HelpExitsSuccessfully) {
  const char* argv[] = {"DxGGenerator", "--help"};

  EXPECT_EXIT(
      {
        const auto input = utils::cli::parse<utils::DxGCli>(2, const_cast<char**>(argv));
        static_cast<void>(input);
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(DxGCliTest, AddOptionsBindsToProvidedInput) {
  CLI::App app;
  utils::DxGAppInput input;
  utils::DxGCli::add_options(app, input);

  ASSERT_NO_THROW(app.parse(
      std::string{"--cc --ee --art --pil --old_format --recurrence-test --nd 2 --popsize 50 "
                  "--iov 0.1 --iiv 0.2 --as_ec50 3.0 --dose 0 1 --halflife 2 3 "
                  "--kmax 0.8 0.9 --EC50 4 5 --slope 1.2 1.3 --mda 0.5 0.6"}));

  EXPECT_TRUE(input.is_crt_calibration);
  EXPECT_TRUE(input.is_ee_calibration);
  EXPECT_TRUE(input.is_art);
  EXPECT_TRUE(input.is_print_immunity_level);
  EXPECT_TRUE(input.is_old_format);
  EXPECT_TRUE(input.is_recurrence_test);
  EXPECT_EQ(input.number_of_drugs_in_combination, 2);
  EXPECT_EQ(input.population_size, 50);
  EXPECT_DOUBLE_EQ(input.as_iov, 0.1);
  EXPECT_DOUBLE_EQ(input.as_iiv, 0.2);
  EXPECT_DOUBLE_EQ(input.as_ec50, 3.0);
  EXPECT_EQ(input.dosing_days, (std::vector<int>{0, 1}));
  EXPECT_EQ(input.half_life, (std::vector<double>{2.0, 3.0}));
  EXPECT_EQ(input.k_max, (std::vector<double>{0.8, 0.9}));
  EXPECT_EQ(input.ec50, (std::vector<double>{4.0, 5.0}));
  EXPECT_EQ(input.slope, (std::vector<double>{1.2, 1.3}));
  EXPECT_EQ(input.mean_drug_absorption, (std::vector<double>{0.5, 0.6}));
}

}  // namespace
