#include <gtest/gtest.h>

#include <algorithm>


#define private public
#include "Configuration/Config.h"
#undef private

#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Environment/SeasonalEquation.h"
#include "Environment/SeasonalRainfall.h"
#include "Spatial/GIS/SpatialData.h"
#include "fixtures/TestFileGenerators.h"

class ConfigValidationEdgeCasesTest : public ::testing::Test {
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
  }

  Config* config() { return Model::get_config(); }
};

TEST_F(ConfigValidationEdgeCasesTest, RejectsAdditionalTimeframeAndDemographicValues) {
  config()->simulation_timeframe_.start_collect_data_day_ = -1;
  EXPECT_THROW(config()->validate_simulation_timeframe(), std::invalid_argument);

  config()->simulation_timeframe_.start_collect_data_day_ = 0;
  config()->population_demographic_.number_of_age_classes_ = 0;
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);

  config()->population_demographic_.number_of_age_classes_ = 6;
  config()->population_demographic_.initial_age_structure_ = {-1};
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);

  config()->population_demographic_.initial_age_structure_ = {};
  config()->population_demographic_.artificial_rescaling_of_population_size_ = 0;
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);

  config()->population_demographic_.artificial_rescaling_of_population_size_ = 1;
  config()->population_demographic_.initial_age_structure_ = {1};
  config()->population_demographic_.death_rate_by_age_class_ = {-1};
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
  config()->population_demographic_.death_rate_by_age_class_ = {1};
  config()->population_demographic_.mortality_when_treatment_fail_by_age_class_ = {-1};
  EXPECT_THROW(config()->validate_population_demographic(), std::invalid_argument);
}

TEST_F(ConfigValidationEdgeCasesTest, RejectsGenotypeAndTherapyParameterRanges) {
  config()->genotype_parameters_.mutation_mask_.clear();
  EXPECT_THROW(config()->validate_genotype_parameters(), std::invalid_argument);

  config()->genotype_parameters_.mutation_mask_ = {true};
  config()->genotype_parameters_.mutation_probability_per_locus_ = 2.0;
  EXPECT_THROW(config()->validate_genotype_parameters(), std::invalid_argument);

  config()->genotype_parameters_.mutation_probability_per_locus_ = 0.1;
  auto patterns = config()->genotype_parameters_.override_ec50_patterns_;
  ASSERT_FALSE(patterns.empty());
  patterns.front().set_pattern("bad");
  config()->genotype_parameters_.override_ec50_patterns_ = patterns;
  EXPECT_THROW(config()->validate_genotype_parameters(), std::invalid_argument);

  auto pipe_invalid_patterns = patterns;
  auto pipe_invalid = pipe_invalid_patterns.front().get_pattern();
  pipe_invalid.assign(pipe_invalid.size(), 'A');
  pipe_invalid_patterns.front().set_pattern(pipe_invalid);
  config()->genotype_parameters_.override_ec50_patterns_ = pipe_invalid_patterns;
  EXPECT_THROW(config()->validate_genotype_parameters(), std::invalid_argument);

  config()->therapy_parameters_.tf_testing_day_ = -1;
  EXPECT_THROW((void)config()->validate_therapy_parameters(), std::invalid_argument);
  config()->therapy_parameters_.tf_testing_day_ = 0;
  config()->therapy_parameters_.tf_rate_ = 2.0;
  EXPECT_THROW((void)config()->validate_therapy_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationEdgeCasesTest, RejectsAdditionalEpidemiologicalRanges) {
  config()->epidemiological_parameters_.number_of_tracking_days_ = 0;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  config()->epidemiological_parameters_.number_of_tracking_days_ = 1;
  config()->epidemiological_parameters_.days_to_clinical_under_five_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  config()->epidemiological_parameters_.days_to_clinical_under_five_ = 1;
  config()->epidemiological_parameters_.gametocyte_level_full_ = 2;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  config()->epidemiological_parameters_.gametocyte_level_full_ = 0.5;
  config()->epidemiological_parameters_.relapse_duration_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
  config()->epidemiological_parameters_.relapse_duration_ = 1;
  config()->epidemiological_parameters_.tf_window_size_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationEdgeCasesTest, RejectsInvalidTherapyAndStrategyDatabaseReferences) {
  auto therapies = config()->therapy_parameters_.therapy_db_raw_;
  ASSERT_FALSE(therapies.empty());
  auto therapy = therapies.begin()->second;
  therapy.set_drug_ids({9999});
  therapies.begin()->second = therapy;
  config()->therapy_parameters_.therapy_db_raw_ = therapies;
  EXPECT_THROW((void)config()->validate_therapy_parameters(), std::invalid_argument);

  config()->therapy_parameters_.therapy_db_raw_ = Model::get_config()->therapy_parameters_.therapy_db_raw_;
  auto strategies = config()->strategy_parameters_.strategy_db_raw_;
  config()->strategy_parameters_.initial_strategy_id_ = 9999;
  EXPECT_THROW(config()->validate_strategy_parameters(), std::invalid_argument);
  config()->strategy_parameters_.initial_strategy_id_ = strategies.begin()->first;
  config()->strategy_parameters_.second_line_strategy_id_ = 9999;
  EXPECT_THROW(config()->validate_strategy_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationEdgeCasesTest, ExercisesConfigurationAccessorGraph) {
  const auto &epi = config()->get_epidemiological_parameters();
  (void)epi.get_number_of_tracking_days();
  (void)epi.get_days_to_clinical_under_five();
  (void)epi.get_days_to_clinical_over_five();
  (void)epi.get_days_mature_gametocyte_under_five();
  (void)epi.get_days_mature_gametocyte_over_five();
  (void)epi.get_p_compliance();
  (void)epi.get_min_dosing_days();
  (void)epi.get_gametocyte_level_under_artemisinin_action();
  (void)epi.get_gametocyte_level_full();
  (void)epi.get_p_relapse();
  (void)epi.get_relapse_duration();
  (void)epi.get_relapse_rate();
  (void)epi.get_update_frequency();
  (void)epi.get_tf_window_size();
  (void)epi.get_fraction_mosquitoes_interrupted_feeding();
  (void)epi.get_inflation_factor();
  (void)epi.get_using_age_dependent_biting_level();
  (void)epi.get_using_variable_probability_infectious_bites_cause_infection();
  const auto &biting = epi.get_relative_biting_info();
  (void)biting.get_max_relative_biting_value();
  (void)biting.get_min_relative_biting_value();
  (void)biting.get_number_of_biting_levels();
  (void)biting.get_biting_level_distribution().get_distribution();
  (void)epi.get_relative_infectivity().get_sigma();
  (void)epi.get_relative_infectivity().get_ro_star();
  (void)epi.get_relative_infectivity().get_blood_meal_volume();
  (void)epi.get_allow_new_coinfection_to_cause_symptoms().get_enable();

  auto &movement = config()->get_movement_settings();
  (void)movement.get_spatial_model_settings().get_name();
  (void)movement.get_circulation_info().get_circulation_percent();
  (void)movement.get_circulation_info().get_max_relative_moving_value();
  (void)movement.get_circulation_info().get_number_of_moving_levels();
  (void)movement.get_circulation_info().get_length_of_stay().get_mean();
  (void)movement.get_circulation_info().get_length_of_stay().get_sd();
  (void)movement.get_length_of_stay_theta();
  (void)movement.get_length_of_stay_k();
  (void)movement.get_v_moving_level_density();
  (void)movement.get_v_moving_level_value();

  auto &genotype = config()->get_genotype_parameters();
  (void)genotype.get_mutation_mask();
  (void)genotype.get_mutation_probability_per_locus();
  (void)genotype.get_default_cnv_reversion_multiplier();
  (void)genotype.get_override_ec50_patterns();
  (void)genotype.get_initial_parasite_info();
  (void)genotype.get_initial_parasite_info_raw();
  (void)genotype.get_pf_genotype_info().get_cnv_gene_indices();
}

TEST_F(ConfigValidationEdgeCasesTest, HandlesMissingConfigurationFile) {
  Config unloaded_config;
  EXPECT_FALSE(unloaded_config.load("config-file-that-does-not-exist.yml"));
}

TEST_F(ConfigValidationEdgeCasesTest, SelectsRandomCalibrationAndHandlesEmptyCandidates) {
  config()->version6_pfpr_incidence_calibrations_.set_calibration_ids({
      {4, ImmuneSystemParametercalibration_id{}},
      {9, ImmuneSystemParametercalibration_id{}},
  });
  config()->version6_pfpr_incidence_calibrations_.set_chosen_calibration_id(-1);
  config()->select_random_immune_system_parameter_calibration_id();
  EXPECT_TRUE(config()->version6_pfpr_incidence_calibrations_.get_chosen_calibration_id() == 4 ||
              config()->version6_pfpr_incidence_calibrations_.get_chosen_calibration_id() == 9);

  config()->version6_pfpr_incidence_calibrations_.set_calibration_ids({});
  EXPECT_NO_THROW(config()->select_random_immune_system_parameter_calibration_id());
}

TEST_F(ConfigValidationEdgeCasesTest, ParsesRandomCalibrationSelectionAndUnknownSelection) {
  const auto calibration_config = YAML::Load(R"(
    chosen_calibration_id: 99
    random_selection: true
    calibration_ids:
      4: {}
      9: {}
  )");
  YAML::Node root;
  root["version6_pfpr_incidence_calibrations"] = calibration_config;
  EXPECT_NO_THROW(config()->parse_version6_pfpr_incidence_calibrations(root));
  EXPECT_TRUE(config()->has_version6_pfpr_incidence_calibrations());
  EXPECT_TRUE(config()->version6_pfpr_incidence_calibrations_.has_selected_calibration_id());

  config()->version6_pfpr_incidence_calibrations_.set_chosen_calibration_id(99);
  EXPECT_NO_THROW(config()->apply_selected_immune_system_parameter_calibration_id());
}

TEST_F(ConfigValidationEdgeCasesTest, RejectsInvalidSeasonalitySettings) {
  config()->seasonality_settings_.set_enable(true);
  config()->seasonality_settings_.set_mode("");
  EXPECT_THROW(config()->validate_seasonality_settings(), std::invalid_argument);

  auto rainfall = std::make_unique<SeasonalRainfall>();
  rainfall->set_filename("missing-rainfall.csv");
  rainfall->set_period(365);
  config()->seasonality_settings_.set_mode("rainfall");
  config()->seasonality_settings_.set_seasonal_rainfall(std::move(rainfall));
  EXPECT_THROW(config()->validate_seasonality_settings(), std::invalid_argument);

  auto equation = std::make_unique<SeasonalEquation>();
  equation->set_raster(true);
  config()->seasonality_settings_.set_mode("equation");
  config()->seasonality_settings_.set_seasonal_equation(std::move(equation));
  config()->spatial_settings_.set_spatial_data(
      std::make_unique<SpatialData>(&config()->spatial_settings_));
  EXPECT_THROW(config()->validate_seasonality_settings(), std::invalid_argument);
}

TEST_F(ConfigValidationEdgeCasesTest, RejectsInvalidPublicPrivateStrategyShares) {
  auto strategies = config()->strategy_parameters_.get_strategy_db_raw();
  auto strategy_it = std::find_if(
      strategies.begin(), strategies.end(), [](const auto &entry) {
        return entry.second.get_type() == "PublicPrivate";
      });
  ASSERT_NE(strategy_it, strategies.end());

  auto invalid_strategy = strategy_it->second;
  invalid_strategy.set_start_public_share(-0.1);
  strategies[strategy_it->first] = invalid_strategy;
  config()->strategy_parameters_.set_strategy_db_raw(strategies);
  EXPECT_THROW(config()->validate_strategy_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationEdgeCasesTest, RejectsInvalidPublicPrivateStrategyReferences) {
  auto strategies = config()->strategy_parameters_.get_strategy_db_raw();
  auto strategy_it = std::find_if(
      strategies.begin(), strategies.end(), [](const auto &entry) {
        return entry.second.get_type() == "PublicPrivate";
      });
  ASSERT_NE(strategy_it, strategies.end());
  const auto original = strategy_it->second;

  auto invalid = original;
  invalid.set_public_strategy_id(0);
  invalid.set_private_strategy_id(0);
  strategy_it->second = invalid;
  config()->strategy_parameters_.set_strategy_db_raw(strategies);
  EXPECT_THROW(config()->validate_strategy_parameters(), std::invalid_argument);

  invalid = original;
  invalid.set_public_strategy_id(strategy_it->first);
  invalid.set_private_strategy_id(0);
  strategy_it->second = invalid;
  config()->strategy_parameters_.set_strategy_db_raw(strategies);
  EXPECT_THROW(config()->validate_strategy_parameters(), std::invalid_argument);

  invalid = original;
  invalid.set_peak_after(-1);
  strategy_it->second = invalid;
  config()->strategy_parameters_.set_strategy_db_raw(strategies);
  EXPECT_THROW(config()->validate_strategy_parameters(), std::invalid_argument);

  invalid.set_start_public_share_by_location(std::vector<double>(8, 0.4));
  invalid.set_peak_public_share_by_location(std::vector<double>(8, 0.6));
  invalid.set_peak_after(original.get_peak_after());
  strategy_it->second = invalid;
  config()->strategy_parameters_.set_strategy_db_raw(strategies);
  EXPECT_NO_THROW(config()->validate_strategy_parameters());
}

TEST_F(ConfigValidationEdgeCasesTest, RejectsInvalidPublicPrivateLocationShares) {
  auto strategies = config()->strategy_parameters_.get_strategy_db_raw();
  auto strategy_it = std::find_if(
      strategies.begin(), strategies.end(), [](const auto &entry) {
        return entry.second.get_type() == "PublicPrivate";
      });
  ASSERT_NE(strategy_it, strategies.end());
  auto invalid = strategy_it->second;
  invalid.set_type("PublicPrivateMultiLocation");
  invalid.set_start_public_share_by_location({0.5});
  invalid.set_peak_public_share_by_location({0.5});
  strategy_it->second = invalid;
  config()->strategy_parameters_.set_strategy_db_raw(strategies);
  EXPECT_THROW(config()->validate_strategy_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationEdgeCasesTest, AppliesCalibrationOverridesAndSkipsNegativeValues) {
  namespace P = ImmuneSystemOverridePaths;
  ImmuneSystemParametercalibration_id overrides;
  overrides.overrides[P::K_Z] = 0.25;
  overrides.overrides[P::K_KAPPA] = 0.75;
  overrides.overrides[P::K_MIDPOINT] = 2.0;
  overrides.overrides[P::K_P_CI_SYMP] = 0.4;
  overrides.overrides[P::K_P_SEEK_BASE] = 0.3;
  overrides.overrides[P::K_MUTATION_PROB] = -1.0;
  overrides.overrides[P::K_DEFAULT_CNV_REVERSION_MULTIPLIER] = -1.0;
  config()->version6_pfpr_incidence_calibrations_.set_chosen_calibration_id(7);
  config()->version6_pfpr_incidence_calibrations_.set_calibration_ids({{7, overrides}});

  EXPECT_NO_THROW(config()->apply_selected_immune_system_parameter_calibration_id());
  EXPECT_DOUBLE_EQ(config()->immune_system_parameters_.get_immune_effect_on_progression_to_clinical(), 0.25);
  EXPECT_DOUBLE_EQ(config()->immune_system_parameters_.get_factor_effect_age_mature_immunity(), 0.75);
  EXPECT_DOUBLE_EQ(config()->immune_system_parameters_.get_midpoint(), 2.0);
}
