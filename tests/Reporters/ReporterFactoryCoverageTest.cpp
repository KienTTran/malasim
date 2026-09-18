#include <gtest/gtest.h>

#include <algorithm>

#include "Reporters/Reporter.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

TEST(ReporterFactoryCoverageTest, ListsSupportedNamesAndRejectsUnimplementedTypes) {
  const auto names = Reporter::available_reporters();
  EXPECT_NE(std::find(names.begin(), names.end(), "Console"), names.end());
  EXPECT_NE(std::find(names.begin(), names.end(), "SQLiteMonthlyReporter"), names.end());
  EXPECT_EQ(Reporter::make_report(Reporter::ReportType::MOVEMENT_REPORTER), nullptr);
  EXPECT_EQ(Reporter::make_report(Reporter::ReportType::GENOTYPE_CARRIERS), nullptr);
  EXPECT_EQ(Reporter::make_report(Reporter::ReportType::THERAPY_RECORD_REPORTER), nullptr);
}

TEST(ReporterFactoryCoverageTest, ConstructsSqliteReportersWithModelConfiguration) {
  test_fixtures::setup_test_environment();
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());

  EXPECT_NE(Reporter::make_report(Reporter::ReportType::SQLITE_MONTHLY_REPORTER), nullptr);
  EXPECT_NE(Reporter::make_report(Reporter::ReportType::SQLITE_VALIDATION_REPORTER), nullptr);

  Model::get_instance()->release();
  test_fixtures::cleanup_test_files();
}
