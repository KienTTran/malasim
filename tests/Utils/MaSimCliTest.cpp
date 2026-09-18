#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "apps/malasim/MaSimCli.h"

namespace {

static_assert(utils::cli::CliApplication<utils::MaSimCli>);

TEST(MaSimCliParseTest, ParsesDefaultValues) {
  const char* argv[] = {"malasim"};
  const auto input = utils::cli::parse<utils::MaSimCli>(1, const_cast<char**>(argv));

  EXPECT_EQ(input.input_path, "input.yml");
  EXPECT_TRUE(input.output_path.empty());
  EXPECT_TRUE(input.reporter.empty());
  EXPECT_EQ(input.verbosity, 0);
  EXPECT_EQ(input.job_number, 0);
  EXPECT_EQ(input.replicate, 1);
  EXPECT_FALSE(input.list_reporters);
  EXPECT_FALSE(input.dump_movement_matrix);
  EXPECT_FALSE(input.record_individual_movement);
  EXPECT_FALSE(input.record_cell_movement);
  EXPECT_FALSE(input.record_district_movement);
  EXPECT_FALSE(input.record_movement);
  EXPECT_FALSE(input.print_memory_stats);
}

TEST(MaSimCliParseTest, ParsesAllOptions) {
  const char* argv[] = {"malasim",
                        "-i",
                        "custom.yml",
                        "-o",
                        "output-dir",
                        "-r",
                        "ReporterA,ReporterB",
                        "-v",
                        "2",
                        "-j",
                        "42",
                        "--replicate",
                        "3",
                        "--dump",
                        "--list",
                        "--im",
                        "--mc",
                        "--memory-stats"};
  const auto input = utils::cli::parse<utils::MaSimCli>(static_cast<int>(std::size(argv)),
                                                        const_cast<char**>(argv));

  EXPECT_EQ(input.input_path, "custom.yml");
  EXPECT_EQ(input.output_path, "output-dir");
  EXPECT_EQ(input.reporter, "ReporterA,ReporterB");
  EXPECT_EQ(input.verbosity, 2);
  EXPECT_EQ(input.job_number, 42);
  EXPECT_EQ(input.replicate, 3);
  EXPECT_TRUE(input.dump_movement_matrix);
  EXPECT_TRUE(input.list_reporters);
  EXPECT_TRUE(input.record_individual_movement);
  EXPECT_TRUE(input.record_cell_movement);
  EXPECT_FALSE(input.record_district_movement);
  EXPECT_TRUE(input.print_memory_stats);
}

TEST(MaSimCliParseTest, ParsesDistrictMovement) {
  const char* argv[] = {"malasim", "--md"};
  const auto input = utils::cli::parse<utils::MaSimCli>(2, const_cast<char**>(argv));

  EXPECT_TRUE(input.record_district_movement);
}

TEST(MaSimCliParseTest, PreservesCommaSeparatedReporterList) {
  const char* argv[] = {"malasim", "-r", "SQLiteMonthlyReporter,SQLiteValidationReporter"};
  const auto input = utils::cli::parse<utils::MaSimCli>(3, const_cast<char**>(argv));

  EXPECT_EQ(input.reporter, "SQLiteMonthlyReporter,SQLiteValidationReporter");
}

TEST(MaSimCliParseTest, RejectsUnknownOption) {
  const char* argv[] = {"malasim", "--not-a-masim-option"};

  EXPECT_THROW(
      {
        const auto input = utils::cli::parse<utils::MaSimCli>(2, const_cast<char**>(argv));
        static_cast<void>(input);
      },
      CLI::ParseError);
}

TEST(MaSimCliParseTest, HelpExitsSuccessfully) {
  const char* argv[] = {"malasim", "--help"};

  EXPECT_EXIT(
      {
        const auto input = utils::cli::parse<utils::MaSimCli>(2, const_cast<char**>(argv));
        static_cast<void>(input);
      },
      ::testing::ExitedWithCode(0), "");
}

class MaSimCliValidationTest : public ::testing::Test {
protected:
  static constexpr const char* kTempInput = "masim_cli_validation_input.yml";

  void SetUp() override {
    std::ofstream output(kTempInput);
    output << "# temp\n";
  }

  void TearDown() override { std::remove(kTempInput); }
};

TEST_F(MaSimCliValidationTest, CreatesMissingOutputDirectory) {
  const std::string output_path = "masim_cli_output_directory/";
  std::filesystem::remove_all(output_path);

  utils::MaSimAppInput input;
  input.input_path = kTempInput;
  input.output_path = output_path;

  ASSERT_TRUE(utils::cli::validate<utils::MaSimCli>(input));
  EXPECT_TRUE(std::filesystem::is_directory(output_path));
  std::filesystem::remove_all(output_path);
}

TEST_F(MaSimCliValidationTest, TreatsMissingOutputFileAsFilenameOverride) {
  const std::filesystem::path output_path = "masim_cli_output.db";
  std::filesystem::remove(output_path);

  utils::MaSimAppInput input;
  input.input_path = kTempInput;
  input.output_path = output_path.string();

  ASSERT_TRUE(utils::cli::validate<utils::MaSimCli>(input));
  EXPECT_FALSE(std::filesystem::exists(output_path));
}

TEST_F(MaSimCliValidationTest, PreservesExistingOutputFile) {
  const std::filesystem::path output_path = "masim_cli_output.db";
  {
    std::ofstream output(output_path);
    output << "existing database";
  }

  utils::MaSimAppInput input;
  input.input_path = kTempInput;
  input.output_path = output_path.string();

  ASSERT_TRUE(utils::cli::validate<utils::MaSimCli>(input));
  EXPECT_TRUE(std::filesystem::is_regular_file(output_path));
  std::filesystem::remove(output_path);
}

TEST_F(MaSimCliValidationTest, RejectsMissingInputFile) {
  utils::MaSimAppInput input;
  input.input_path = "definitely_missing_cli_input.yml";

  EXPECT_FALSE(utils::cli::validate<utils::MaSimCli>(input));
}

TEST_F(MaSimCliValidationTest, TerminalModesDoNotRequireInputFile) {
  for (const auto list_reporters : {false, true}) {
    utils::MaSimAppInput input;
    input.input_path = "definitely_missing_cli_input.yml";
    input.list_reporters = list_reporters;
    input.print_memory_stats = !list_reporters;

    EXPECT_TRUE(utils::cli::validate<utils::MaSimCli>(input));
  }
}

TEST_F(MaSimCliValidationTest, RejectsCellAndDistrictMovementTogether) {
  utils::MaSimAppInput input;
  input.input_path = kTempInput;
  input.record_cell_movement = true;
  input.record_district_movement = true;

  EXPECT_FALSE(utils::cli::validate<utils::MaSimCli>(input));
}

TEST_F(MaSimCliValidationTest, AggregatesEachMovementMode) {
  for (const auto movement_mode : {0, 1, 2}) {
    utils::MaSimAppInput input;
    input.input_path = kTempInput;
    input.record_individual_movement = movement_mode == 0;
    input.record_cell_movement = movement_mode == 1;
    input.record_district_movement = movement_mode == 2;

    ASSERT_TRUE(utils::cli::validate<utils::MaSimCli>(input));
    EXPECT_TRUE(input.record_movement);
  }
}

TEST_F(MaSimCliValidationTest, LeavesAggregateMovementFalseWhenNoModeIsEnabled) {
  utils::MaSimAppInput input;
  input.input_path = kTempInput;

  ASSERT_TRUE(utils::cli::validate<utils::MaSimCli>(input));
  EXPECT_FALSE(input.record_movement);
}

TEST_F(MaSimCliValidationTest, HandlesEveryVerbosityBranch) {
  for (const auto verbosity : {0, 1, 2, 99}) {
    utils::MaSimAppInput input;
    input.input_path = kTempInput;
    input.verbosity = verbosity;

    EXPECT_TRUE(utils::cli::validate<utils::MaSimCli>(input)) << "verbosity=" << verbosity;
  }
}

TEST(MaSimCliOptionsTest, BindsOptionsToProvidedInput) {
  CLI::App app;
  utils::MaSimAppInput input;
  utils::MaSimCli::add_options(app, input);

  ASSERT_NO_THROW(
      app.parse(std::string{"-i in.yml -o /tmp/o -r MMC -v 1 -j 4 --replicate 2 "
                            "--dump --list --im --mc --md --memory-stats"}));

  EXPECT_EQ(input.input_path, "in.yml");
  EXPECT_EQ(input.output_path, "/tmp/o");
  EXPECT_EQ(input.reporter, "MMC");
  EXPECT_EQ(input.verbosity, 1);
  EXPECT_EQ(input.job_number, 4);
  EXPECT_EQ(input.replicate, 2);
  EXPECT_TRUE(input.dump_movement_matrix);
  EXPECT_TRUE(input.list_reporters);
  EXPECT_TRUE(input.record_individual_movement);
  EXPECT_TRUE(input.record_cell_movement);
  EXPECT_TRUE(input.record_district_movement);
  EXPECT_TRUE(input.print_memory_stats);
}

}  // namespace
