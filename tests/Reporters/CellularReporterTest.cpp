#include <gtest/gtest.h>

#include <cstdio>

#include "Configuration/Config.h"
#include "Reporters/Specialist/CellularReporter.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class CellularReporterTest : public ::testing::Test {
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
    std::remove(".cellular_aggregate_data_0.csv");
    std::remove(".cellular_detailed_data_0.csv");
    std::remove(".cellular_blood_data_0.csv");
  }
};

TEST_F(CellularReporterTest, InitializesAndReportsSingleLocationData) {
  // CellularReporter is intentionally restricted to a single cell. The model
  // fixture has multiple cells, so reduce the reporter's configured view after
  // initialization while retaining the populated index for cell zero.
  Model::get_config()->location_db().resize(1);

  CellularReporter reporter;
  EXPECT_NO_THROW(reporter.initialize(0, ".cellular"));
  EXPECT_NO_THROW(reporter.monthly_report());
}
