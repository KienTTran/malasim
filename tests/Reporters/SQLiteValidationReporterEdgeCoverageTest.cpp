#include <gtest/gtest.h>

#include "Reporters/SQLiteValidationReporter.h"
#include "Reporters/Reporter.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class SQLiteValidationReporterEdgeCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment("test_input.yml");
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    cli_input.output_path = ".";
    cli_input.reporter = "SQLiteValidationReporter";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(SQLiteValidationReporterEdgeCoverageTest, ReportsCellLevelSiteAndGenomeData) {
  ASSERT_EQ(Model::get_instance()->get_reporters().size(), 1U);
  auto* reporter = dynamic_cast<SQLiteValidationReporter*>(
      Model::get_instance()->get_reporters().front().get());
  ASSERT_NE(reporter, nullptr);
  reporter->set_cell_level_reporting(true);

  EXPECT_NO_THROW(reporter->monthly_report_site_data(1));
  EXPECT_NO_THROW(reporter->monthly_report_genome_data(1));
}
