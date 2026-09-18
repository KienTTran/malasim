#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Reporters/Reporter.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Logger.h"
#include "fixtures/TestFileGenerators.h"

namespace {

class ScopedCoutSilencer {
public:
  ScopedCoutSilencer() : original_buffer_(std::cout.rdbuf(sink_.rdbuf())) {}
  ~ScopedCoutSilencer() { std::cout.rdbuf(original_buffer_); }

private:
  std::ostringstream sink_;
  std::streambuf* original_buffer_;
};

void silence_spdlog() {
  spdlog::set_level(spdlog::level::off);
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) {
    logger->set_level(spdlog::level::off);
  });
}

}  // namespace

// Runs a short full simulation once per reporter type through Model::run().
// This exercises the reporter construction/factory path, the reporter's
// initialize/begin/monthly/end lifecycle, and the Scheduler/Model/MDC daily,
// monthly, and yearly update chain that feeds reporters their data.
class ReporterRunIntegrationTest
    : public ::testing::TestWithParam<std::string> {
protected:
  void TearDown() override {
    if (Model::get_instance() != nullptr) { Model::get_instance()->release(); }
    spdlog::drop_all();
    Logger::initialize(spdlog::level::off);
    test_fixtures::cleanup_test_files();
  }
};

TEST_P(ReporterRunIntegrationTest, RunsShortSimulationWithReporter) {
  ScopedCoutSilencer silence_console_output;

  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node& cfg) {
    cfg["simulation_timeframe"]["starting_date"] = "2000/1/1";
    cfg["simulation_timeframe"]["ending_date"] = "2000/3/1";
    cfg["model_settings"]["initial_seed_number"] = 42;
    if (GetParam() == "NovelDrug") {
      cfg["strategy_parameters"]["initial_strategy_id"] = 16;
    }
  });

  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.output_path = ".";
  cli_input.reporter = GetParam();
  Model::set_cli_input(cli_input);

  silence_spdlog();
  ASSERT_TRUE(Model::get_instance()->initialize());
  silence_spdlog();
  ASSERT_NO_THROW(Model::get_instance()->run());
}

INSTANTIATE_TEST_SUITE_P(
    AllReporters, ReporterRunIntegrationTest,
    ::testing::Values("Console", "MonthlyReporter", "MMC", "TACT", "ValidationReporter",
                      "SQLiteMonthlyReporter",
                      "SQLiteValidationReporter", "PopulationReporter", "AgeBand",
                      "SeasonalImmunity", "NovelDrug"),
    [](const ::testing::TestParamInfo<std::string>& info) { return info.param; });

// Stacks several reporters in a single run via add_reporter() so that multiple
// reporter monthly_report() paths execute against one shared simulation.
TEST(ReporterStackTest, MultipleReportersInSingleRun) {
  ScopedCoutSilencer silence_console_output;

  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node& cfg) {
    cfg["simulation_timeframe"]["starting_date"] = "2000/1/1";
    cfg["simulation_timeframe"]["ending_date"] = "2000/2/1";
    cfg["model_settings"]["initial_seed_number"] = 7;
  });

  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.output_path = ".";
  cli_input.reporter = "Console";
  Model::set_cli_input(cli_input);

  silence_spdlog();
  ASSERT_TRUE(Model::get_instance()->initialize());

  for (auto type : {Reporter::ReportType::MONTHLY_REPORTER,
                    Reporter::ReportType::POPULATION_REPORTER,
                    Reporter::ReportType::AGE_BAND_REPORTER}) {
    auto extra = Reporter::make_report(type);
    ASSERT_NE(extra, nullptr);
    extra->initialize(cli_input.job_number, cli_input.output_path);
    Model::get_instance()->add_reporter(std::move(extra));
  }

  silence_spdlog();
  ASSERT_NO_THROW(Model::get_instance()->run());

  Model::get_instance()->release();
  test_fixtures::cleanup_test_files();
}

// ---------------------------------------------------------------------------
// Reporter / strategy mismatch
//
// Some reporters are written for one particular strategy type but nothing stops
// a configuration from pairing them with another. NovelDrugReporter used to
// dereference the result of dynamic_cast<NovelDrugIntroductionStrategy*>
// without checking it, so any other active strategy was a null dereference in
// both monthly_report() and after_run(). Note that the parameterised suite
// above works around exactly this by forcing initial_strategy_id to 16 for the
// NovelDrug case; these tests deliberately do not.
//
// TACTReporter guarded its cast with a type check, but skipped the columns
// entirely on a mismatch, so the row width silently depended on the active
// strategy. It now emits placeholders instead.
// ---------------------------------------------------------------------------

namespace {

// Adds a DistrictMFT strategy covering the three districts that
// test_fixtures::create_test_district_raster generates, and selects it.
void install_district_mft_strategy(YAML::Node& cfg, int strategy_id) {
  const std::string key = std::to_string(strategy_id);
  cfg["strategy_parameters"]["strategy_db"][key]["name"] = "TestDistrictMFT";
  cfg["strategy_parameters"]["strategy_db"][key]["type"] = "DistrictMFT";
  cfg["strategy_parameters"]["strategy_db"][key]["definitions"]["0"]["district_ids"] =
      std::vector<int>{1, 2, 3};
  cfg["strategy_parameters"]["strategy_db"][key]["definitions"]["0"]["therapy_ids"] =
      std::vector<int>{6, 8};
  cfg["strategy_parameters"]["strategy_db"][key]["definitions"]["0"]["distribution"] =
      std::vector<double>{0.5, 0.5};
  cfg["strategy_parameters"]["initial_strategy_id"] = strategy_id;
  // The second-line strategy is exercised for recurrent cases; leave it on a
  // simple strategy so this test isolates the reporter behaviour.
  cfg["strategy_parameters"]["second_line_strategy_id"] = 2;
}

constexpr int K_DISTRICT_MFT_STRATEGY_ID = 17;

}  // namespace

class ReporterStrategyMismatchTest
    : public ::testing::TestWithParam<std::pair<std::string, std::string>> {
protected:
  void TearDown() override {
    if (Model::get_instance() != nullptr) { Model::get_instance()->release(); }
    spdlog::drop_all();
    Logger::initialize(spdlog::level::off);
    test_fixtures::cleanup_test_files();
  }
};

TEST_P(ReporterStrategyMismatchTest, SurvivesIncompatibleTreatmentStrategy) {
  ScopedCoutSilencer silence_console_output;
  const auto& [reporter_name, strategy_kind] = GetParam();

  test_fixtures::setup_test_environment(
      "test_input.yml", [&strategy_kind](YAML::Node& cfg) {
        cfg["simulation_timeframe"]["starting_date"] = "2000/1/1";
        cfg["simulation_timeframe"]["ending_date"] = "2000/3/1";
        cfg["model_settings"]["initial_seed_number"] = 42;
        if (strategy_kind == "DistrictMFT") {
          install_district_mft_strategy(cfg, K_DISTRICT_MFT_STRATEGY_ID);
        } else {
          // Strategy 0 is a plain MFT: neither a NovelDrugIntroductionStrategy
          // nor a NestedMFTStrategy, so both dynamic_casts return nullptr.
          cfg["strategy_parameters"]["initial_strategy_id"] = 0;
        }
      });

  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.output_path = ".";
  cli_input.reporter = reporter_name;
  Model::set_cli_input(cli_input);

  silence_spdlog();
  ASSERT_TRUE(Model::get_instance()->initialize());
  silence_spdlog();
  // Covers monthly_report() and after_run(); both previously dereferenced null.
  ASSERT_NO_THROW(Model::get_instance()->run());
}

INSTANTIATE_TEST_SUITE_P(
    IncompatibleStrategies, ReporterStrategyMismatchTest,
    ::testing::Values(std::make_pair("NovelDrug", "DistrictMFT"),
                      std::make_pair("NovelDrug", "MFT"),
                      std::make_pair("TACT", "DistrictMFT"),
                      std::make_pair("TACT", "MFT")),
    [](const ::testing::TestParamInfo<std::pair<std::string, std::string>>& info) {
      return info.param.first + "_with_" + info.param.second;
    });
