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
        EpidemiologicalParameters::AllowNewCoinfectionToCauseSymptoms coinfection;
        coinfection.set_enable(true);
        coinfection.set_probability(0.5);
        epi_parameters.set_allow_new_coinfection_to_cause_symptoms(coinfection);
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
    node["allow_new_coinfection_to_cause_symptoms"]["enable"] = true;
    node["allow_new_coinfection_to_cause_symptoms"]["probability"] = 0.5;
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

// Test for AgeBasedProbabilityOfSeekingTreatment: we only support 'power' type now
TEST_F(EpidemiologicalParametersYAMLTest, AgeBased_Power_EvaluateForAgeBasic) {
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("power");
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::PowerConfig pc;
    pc.base = 0.9;
    pc.exponent_source = "index";
    cfg.set_power(pc);
    cfg.set_ages(std::vector<int>{0,5,10,15});

    // Age 2 -> bin idx 0 -> exponent 0 -> multiplier = 0.9^0 = 1.0
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_age(2), 1.0);
    // Age 7 -> idx 1 -> exponent 1 -> multiplier = 0.9^1 = 0.9
    EXPECT_NEAR(cfg.evaluate_for_age(7), 0.9, 1e-12);
    // Age 12 -> idx 2 -> exponent 2 -> multiplier = 0.9^2 = 0.81
    EXPECT_NEAR(cfg.evaluate_for_age(12), 0.81, 1e-12);
}

TEST_F(EpidemiologicalParametersYAMLTest, AgeBased_Disabled_ReturnsOne) {
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(false);
    cfg.set_type("power");
    EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::PowerConfig pc;
    pc.base = 0.9;
    pc.exponent_source = "index";
    cfg.set_power(pc);
    cfg.set_ages(std::vector<int>{0,5,10,15});

    EXPECT_DOUBLE_EQ(cfg.evaluate_for_age(0), 1.0);
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_age(10), 1.0);
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_age(100), 1.0);
}

// ---- PfPRBasedProbabilityOfSeekingTreatment tests ----

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_Disabled_ReturnsOne) {
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(false);

    EXPECT_DOUBLE_EQ(cfg.evaluate_for_pfpr(0.0), 1.0);
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_pfpr(0.5), 1.0);
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_pfpr(1.0), 1.0);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_SaturatingIncrease_ZeroPfPR) {
    // At pfpr=0: modifier = 1 + amplitude*(1 - exp(0)) = 1 + amplitude*0 = 1.0
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.5;
    sc.rate = 2.0;
    sc.max_modifier = 1.5;
    cfg.set_saturating_increase(sc);

    EXPECT_DOUBLE_EQ(cfg.evaluate_for_pfpr(0.0), 1.0);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_SaturatingIncrease_HighPfPR_ClampsToMax) {
    // At pfpr=1 with high rate, exp(-rate) ~ 0, so modifier ~ 1 + amplitude = 1.5
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.5;
    sc.rate = 100.0;   // very steep: saturates almost immediately
    sc.max_modifier = 1.5;
    cfg.set_saturating_increase(sc);

    EXPECT_NEAR(cfg.evaluate_for_pfpr(1.0), 1.5, 1e-6);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_SaturatingIncrease_MidPfPR) {
    // Manually compute: pfpr=0.5, amplitude=0.4, rate=1.0
    // modifier = 1 + 0.4*(1 - exp(-0.5)) = 1 + 0.4*(1 - 0.60653) = 1 + 0.4*0.39347 = 1.15739
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.4;
    sc.rate = 1.0;
    sc.max_modifier = 2.0;
    cfg.set_saturating_increase(sc);

    const double expected = 1.0 + 0.4 * (1.0 - std::exp(-0.5));
    EXPECT_NEAR(cfg.evaluate_for_pfpr(0.5), expected, 1e-10);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_MaxModifierCap) {
    // max_modifier set lower than theoretical max — result must be clamped
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.5;
    sc.rate = 100.0;
    sc.max_modifier = 1.2;   // cap below amplitude+1=1.5
    cfg.set_saturating_increase(sc);

    EXPECT_DOUBLE_EQ(cfg.evaluate_for_pfpr(1.0), 1.2);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_PfPRIsPercent) {
    // pfpr_is_percent=true: input 50 should behave like pfpr=0.5
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    cfg.set_pfpr_is_percent(true);
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.4;
    sc.rate = 1.0;
    sc.max_modifier = 2.0;
    cfg.set_saturating_increase(sc);

    const double expected = 1.0 + 0.4 * (1.0 - std::exp(-0.5));
    EXPECT_NEAR(cfg.evaluate_for_pfpr(50.0), expected, 1e-10);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_PfPRClampsToZeroOne) {
    // Negative pfpr -> clamped to 0 -> result = 1.0
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.5;
    sc.rate = 2.0;
    sc.max_modifier = 1.5;
    cfg.set_saturating_increase(sc);

    EXPECT_DOUBLE_EQ(cfg.evaluate_for_pfpr(-0.5), 1.0);
    // pfpr > 1 -> clamped to 1 -> same as pfpr=1 result
    const double at_one = cfg.evaluate_for_pfpr(1.0);
    EXPECT_DOUBLE_EQ(cfg.evaluate_for_pfpr(5.0), at_one);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_Validate_Throws_NegativeAmplitude) {
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = -0.1;
    sc.rate = 1.0;
    sc.max_modifier = 1.5;
    cfg.set_saturating_increase(sc);

    EXPECT_THROW(cfg.validate(), std::runtime_error);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_Validate_Throws_MaxModifierBelowOne) {
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.5;
    sc.rate = 1.0;
    sc.max_modifier = 0.8;   // invalid: < 1.0
    cfg.set_saturating_increase(sc);

    EXPECT_THROW(cfg.validate(), std::runtime_error);
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_Validate_Disabled_DoesNotThrow) {
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(false);
    // Even with bad values, disabled config should not throw
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = -1.0;
    sc.rate = -1.0;
    sc.max_modifier = 0.0;
    cfg.set_saturating_increase(sc);

    EXPECT_NO_THROW(cfg.validate());
}

TEST_F(EpidemiologicalParametersYAMLTest, PfPRBased_YAML_EncodeDecode) {
    // Build an EpidemiologicalParameters with pfpr_based enabled and round-trip via YAML
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment cfg;
    cfg.set_enabled(true);
    cfg.set_type("saturating_increase");
    cfg.set_pfpr_is_percent(false);
    EpidemiologicalParameters::PfPRBasedProbabilityOfSeekingTreatment::SaturatingIncreaseConfig sc;
    sc.amplitude = 0.3;
    sc.rate = 2.5;
    sc.max_modifier = 1.3;
    cfg.set_saturating_increase(sc);
    epi_parameters.set_pfpr_based_probability_of_seeking_treatment(cfg);
    epi_parameters.set_using_age_dependent_biting_level(false);
    epi_parameters.set_using_variable_probability_infectious_bites_cause_infection(false);

    YAML::Node node = YAML::convert<EpidemiologicalParameters>::encode(epi_parameters);

    ASSERT_TRUE(node["pfpr_based_probability_of_seeking_treatment"]);
    const auto n = node["pfpr_based_probability_of_seeking_treatment"];
    EXPECT_EQ(n["enable"].as<bool>(), true);
    EXPECT_EQ(n["type"].as<std::string>(), "saturating_increase");
    EXPECT_DOUBLE_EQ(n["saturating_increase"]["amplitude"].as<double>(), 0.3);
    EXPECT_DOUBLE_EQ(n["saturating_increase"]["rate"].as<double>(), 2.5);
    EXPECT_DOUBLE_EQ(n["saturating_increase"]["max_modifier"].as<double>(), 1.3);
}

