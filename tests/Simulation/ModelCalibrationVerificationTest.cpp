#include <gtest/gtest.h>

#include "Simulation/Model.h"
#include "Treatment/LinearTCM.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ModelCalibrationVerificationTest : public ::testing::Test {
protected:
  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(ModelCalibrationVerificationTest, BeforeRunVerifiesAllCalibrationOverridePaths) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["version6_pfpr_incidence_calibrations"] = YAML::Load(R"(
      chosen_calibration_id: 0
      random_selection: false
      calibration_ids:
        0:
          "immune_system_parameters:immune_effect_on_progression_to_clinical": 1.2
          "immune_system_parameters:factor_effect_age_mature_immunity": 0.4
          "immune_system_parameters:midpoint": 0.8
          "epidemiological_parameters:allow_new_coinfection_to_cause_symptoms:probability": 0.2
          "epidemiological_parameters:age_based_probability_of_seeking_treatment:power:base": 0.7
          "genotype_parameters:mutation_probability_per_locus": 0.0005
          "genotype_parameters:default_cnv_reversion_multiplier": 0.2
          "unknown:field": 7.0
    )");
  });

  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.reporter = "Console";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());
  EXPECT_NO_THROW(Model::get_instance()->before_run());
}

TEST_F(ModelCalibrationVerificationTest, ReleaseResetsInitializationGuard) {
  test_fixtures::setup_test_environment("test_input.yml");
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.reporter = "Console";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());

  Model::get_instance()->release();
  EXPECT_THROW(Model::get_instance()->run(), std::runtime_error);
}

TEST_F(ModelCalibrationVerificationTest, RejectsUnknownAndDuplicateReporters) {
  test_fixtures::setup_test_environment("test_input.yml");
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.reporter = "NoSuchReporter";
  Model::set_cli_input(cli_input);
  EXPECT_FALSE(Model::get_instance()->initialize());

  Model::get_instance()->release();
  cli_input.reporter = "Console, Console";
  Model::set_cli_input(cli_input);
  EXPECT_TRUE(Model::get_instance()->initialize());
  ASSERT_EQ(Model::get_instance()->get_reporters().size(), 1U);
  Model::get_instance()->add_reporter(nullptr);
  EXPECT_EQ(Model::get_instance()->get_reporters().size(), 1U);
}

TEST_F(ModelCalibrationVerificationTest, SQLiteReporterPersistsSelectedCalibrationOverrides) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["version6_pfpr_incidence_calibrations"] = YAML::Load(R"(
      chosen_calibration_id: 0
      random_selection: false
      calibration_ids:
        0:
          "immune_system_parameters:midpoint": 0.8
    )");
  });
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.reporter = "SQLiteMonthlyReporter";
  cli_input.output_path = ".";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());
  ASSERT_EQ(Model::get_instance()->get_reporters().size(), 1U);
}

TEST_F(ModelCalibrationVerificationTest, BeforeRunWarnsWhenChosenCalibrationIsMissing) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["version6_pfpr_incidence_calibrations"] = YAML::Load(R"(
      chosen_calibration_id: 99
      random_selection: false
      calibration_ids:
        0:
          "immune_system_parameters:midpoint": 0.8
    )");
  });
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.reporter = "Console";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());
  EXPECT_NO_THROW(Model::get_instance()->before_run());
}

TEST_F(ModelCalibrationVerificationTest, BeforeRunReportsNegativeOverridesAsKeptDefaults) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["version6_pfpr_incidence_calibrations"] = YAML::Load(R"(
      chosen_calibration_id: 0
      random_selection: false
      calibration_ids:
        0:
          "genotype_parameters:mutation_probability_per_locus": -1.0
    )");
  });
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.reporter = "Console";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());
  EXPECT_NO_THROW(Model::get_instance()->before_run());
}

TEST_F(ModelCalibrationVerificationTest, UpdatesLinearCoverageRateWhenReplacingCoverageModel) {
  test_fixtures::setup_test_environment("test_input.yml");
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  cli_input.reporter = "Console";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());

  auto linear = std::make_unique<LinearTCM>();
  linear->p_treatment_under_5_to.assign(Model::get_config()->number_of_locations(), 0.8);
  linear->p_treatment_over_5_to.assign(Model::get_config()->number_of_locations(), 0.9);
  linear->end_time = 30;
  Model::get_instance()->set_treatment_coverage(std::move(linear));
  ASSERT_NE(Model::get_treatment_coverage(), nullptr);
  EXPECT_EQ(dynamic_cast<LinearTCM*>(Model::get_treatment_coverage())->rate_of_change_under_5.size(),
            Model::get_config()->number_of_locations());
}
