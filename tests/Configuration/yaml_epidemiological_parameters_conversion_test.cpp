#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "Configuration/EpidemiologicalParameters.h"

class EpidemiologicalParametersYAMLTest : public ::testing::Test {
protected:
    EpidemiologicalParameters epi_parameters;

    void SetUp() override {
        // Initialize GammaDistribution
        EpidemiologicalParameters::BitingLevelDistributionGamma gamma;
        gamma.set_mean(5.0);
        gamma.set_sd(1.5);

        // Initialize ExponentialDistribution
        EpidemiologicalParameters::BitingLevelDistributionExponential exponential;
        exponential.set_scale(2.5);

        // Initialize BitingLevelDistribution
        EpidemiologicalParameters::BitingLevelDistribution biting_level;
        biting_level.set_distribution("Gamma");
        biting_level.set_gamma(gamma);
        biting_level.set_exponential(exponential);

        // Initialize RelativeBitingInfo
        EpidemiologicalParameters::RelativeBitingInfo biting_info;
        biting_info.set_max_relative_biting_value(10);
        biting_info.set_min_relative_biting_value(0.5);
        biting_info.set_number_of_biting_levels(3);
        biting_info.set_biting_level_distribution(biting_level);
        biting_info.set_scale(1.0);
        biting_info.set_mean(5.0);
        biting_info.set_sd(1.5);

        // Initialize RelativeInfectivity
        EpidemiologicalParameters::RelativeInfectivity infectivity;
        infectivity.set_sigma(1.2);
        infectivity.set_ro_star(0.8);
        infectivity.set_blood_meal_volume(2.0);

        // Set values in EpidemiologicalParameters
        epi_parameters.set_number_of_tracking_days(365);
        epi_parameters.set_days_to_clinical_under_five(7);
        epi_parameters.set_days_to_clinical_over_five(14);
        epi_parameters.set_days_mature_gametocyte_under_five(10);
        epi_parameters.set_days_mature_gametocyte_over_five(15);
        epi_parameters.set_p_compliance(0.9);
        epi_parameters.set_min_dosing_days(3);
        epi_parameters.set_relative_biting_info(biting_info);
        epi_parameters.set_gametocyte_level_under_artemisinin_action(0.3);
        epi_parameters.set_gametocyte_level_full(1.0);
        epi_parameters.set_relative_infectivity(infectivity);
        epi_parameters.set_p_relapse(0.2);
        epi_parameters.set_relapse_duration(180);
        epi_parameters.set_relapse_rate(0.05);
        epi_parameters.set_update_frequency(30);
        epi_parameters.set_allow_new_coinfection_to_cause_symptoms(true);
        epi_parameters.set_tf_window_size(10);
        epi_parameters.set_fraction_mosquitoes_interrupted_feeding(0.1);
        epi_parameters.set_inflation_factor(1.5);
    }
};

// Test encoding functionality for EpidemiologicalParameters
TEST_F(EpidemiologicalParametersYAMLTest, EncodeEpidemiologicalParameters) {
    YAML::Node node = YAML::convert<EpidemiologicalParameters>::encode(epi_parameters);

    // Validate encoding
    EXPECT_EQ(node["number_of_tracking_days"].as<int>(), 365);
    EXPECT_EQ(node["days_to_clinical_under_five"].as<int>(), 7);
    EXPECT_EQ(node["days_to_clinical_over_five"].as<int>(), 14);
    EXPECT_EQ(node["p_compliance"].as<double>(), 0.9);
    EXPECT_EQ(node["min_dosing_days"].as<int>(), 3);
    EXPECT_EQ(node["relative_biting_info"]["max_relative_biting_value"].as<int>(), 10);
    EXPECT_EQ(node["relative_infectivity"]["sigma"].as<double>(), 1.2);
    EXPECT_EQ(node["p_relapse"].as<double>(), 0.2);
}

// Test decoding functionality for EpidemiologicalParameters
TEST_F(EpidemiologicalParametersYAMLTest, DecodeEpidemiologicalParameters) {
    YAML::Node node;
    node["number_of_tracking_days"] = 365;
    node["days_to_clinical_under_five"] = 7;
    node["days_to_clinical_over_five"] = 14;
    node["days_mature_gametocyte_under_five"] = 10;
    node["days_mature_gametocyte_over_five"] = 15;
    node["p_compliance"] = 0.9;
    node["min_dosing_days"] = 3;
    node["relative_biting_info"]["max_relative_biting_value"] = 10;
    node["relative_biting_info"]["min_relative_biting_value"] = 0.5;
    node["relative_biting_info"]["number_of_biting_levels"] = 3;
    node["relative_biting_info"]["biting_level_distribution"]["distribution"] = "Gamma";
    node["relative_biting_info"]["biting_level_distribution"]["Gamma"]["mean"] = 5.0;
    node["relative_biting_info"]["biting_level_distribution"]["Gamma"]["sd"] = 1.5;
    node["relative_biting_info"]["biting_level_distribution"]["Exponential"]["scale"] = 2.5;
    node["gametocyte_level_under_artemisinin_action"] = 0.3;
    node["gametocyte_level_full"] = 1.0;
    node["relative_infectivity"]["sigma"] = 1.2;
    node["relative_infectivity"]["ro"] = 0.8;
    node["relative_infectivity"]["blood_meal_volume"] = 2.0;
    node["p_relapse"] = 0.2;
    node["relapse_duration"] = 180;
    node["relapse_rate"] = 0.05;
    node["update_frequency"] = 30;
    node["allow_new_coinfection_to_cause_symptoms"] = true;
    node["tf_window_size"] = 10;
    node["fraction_mosquitoes_interrupted_feeding"] = 0.1;
    node["inflation_factor"] = 1.5;
    node["using_age_dependent_biting_level"] = false;
    node["using_variable_probability_infectious_bites_cause_infection"] = false;

    EpidemiologicalParameters decoded_parameters;
    EXPECT_NO_THROW(YAML::convert<EpidemiologicalParameters>::decode(node, decoded_parameters));

    // Validate decoding
    EXPECT_EQ(decoded_parameters.get_number_of_tracking_days(), 365);
    EXPECT_EQ(decoded_parameters.get_days_to_clinical_under_five(), 7);
    EXPECT_EQ(decoded_parameters.get_p_compliance(), 0.9);
    EXPECT_EQ(decoded_parameters.get_relative_biting_info().get_max_relative_biting_value(), 10);
    EXPECT_EQ(decoded_parameters.get_relative_infectivity().get_sigma(), 1.2);
}

// Test for decoding with missing fields
TEST_F(EpidemiologicalParametersYAMLTest, DecodeEpidemiologicalParametersMissingField) {
    YAML::Node node;
    node["number_of_tracking_days"] = 365;  // Missing other fields

    EpidemiologicalParameters decoded_parameters;
    EXPECT_THROW(YAML::convert<EpidemiologicalParameters>::decode(node, decoded_parameters), std::runtime_error);
}

// ----- Added tests for AgeBasedProbabilityOfSeekingTreatment -----

TEST_F(EpidemiologicalParametersYAMLTest, AgeBased_Logistic_EvaluateForAgeMatchesManual) {
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("logistic");
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::LogisticConfig lc;
    lc.min = 0.2;
    lc.max = 0.9;
    lc.midpoint = 20.0;
    lc.k = 0.1;
    cfg.set_logistic(lc);

    // young age: expect value near max
    double v_young = cfg.evaluate_for_age(0);
    double expected_young = lc.min + (lc.max - lc.min) / (1.0 + std::exp(lc.k * (0.0 - lc.midpoint)));
    EXPECT_NEAR(v_young, expected_young, 1e-12);
    EXPECT_GT(v_young, 0.5);

    // midpoint age: expect (min + (max-min)/2)
    double v_mid = cfg.evaluate_for_age(static_cast<int>(lc.midpoint));
    double expected_mid = lc.min + (lc.max - lc.min) / 2.0;
    EXPECT_NEAR(v_mid, expected_mid, 1e-12);

    // old age: expect value near min
    double v_old = cfg.evaluate_for_age(100);
    double expected_old = lc.min + (lc.max - lc.min) / (1.0 + std::exp(lc.k * (100.0 - lc.midpoint)));
    EXPECT_NEAR(v_old, expected_old, 1e-12);
    EXPECT_LE(v_old, 0.3);
}

TEST_F(EpidemiologicalParametersYAMLTest, AgeBased_ExpFloor_EvaluateForAgeMatchesManual) {
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("exp_floor");
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::ExpFloorConfig ec;
    ec.min = 0.25;
    ec.lambda = 0.05;
    cfg.set_exp_floor(ec);

    // Age 0 -> 1.0
    double v0 = cfg.evaluate_for_age(0);
    double expected0 = ec.min + (1.0 - ec.min) * std::exp(-ec.lambda * 0.0);
    EXPECT_NEAR(v0, expected0, 1e-12);
    EXPECT_NEAR(v0, 1.0, 1e-12);

    // Age 20 -> decayed value
    double age = 20.0;
    double v20 = cfg.evaluate_for_age(static_cast<int>(age));
    double expected20 = ec.min + (1.0 - ec.min) * std::exp(-ec.lambda * age);
    EXPECT_NEAR(v20, expected20, 1e-12);
    EXPECT_GT(v20, ec.min);

    // Very large age -> tends to min
    double v_old = cfg.evaluate_for_age(1000);
    double expected_old = ec.min + (1.0 - ec.min) * std::exp(-ec.lambda * 1000.0);
    EXPECT_NEAR(v_old, expected_old, 1e-12);
    EXPECT_NEAR(v_old, ec.min, 1e-6);
}

TEST_F(EpidemiologicalParametersYAMLTest, AgeBased_Disabled_ReturnsOne) {
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(false);
    cfg.set_type("logistic");
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::LogisticConfig lc;
    lc.min = 0.2; lc.max = 0.9; lc.midpoint = 15.0; lc.k = 0.2;
    cfg.set_logistic(lc);

    EXPECT_DOUBLE_EQ(cfg.evaluate_for_age(0), 1.0);
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_age(10), 1.0);
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_age(100), 1.0);
}
