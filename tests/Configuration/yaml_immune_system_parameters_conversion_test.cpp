#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "Configuration/ImmuneSystemParameters.h"

class ImmuneSystemParametersYAMLTest : public ::testing::Test {
protected:
    ImmuneSystemParameters immune_parameters;

    void SetUp() override {
        immune_parameters.set_b1(0.1);
        immune_parameters.set_b2(0.2);
        immune_parameters.set_duration_for_naive(30);
        immune_parameters.set_duration_for_fully_immune(120);
        immune_parameters.set_mean_initial_condition(0.5);
        immune_parameters.set_sd_initial_condition(0.05);
        immune_parameters.set_immune_inflation_rate(1.1);
        immune_parameters.set_min_clinical_probability(0.01);
        immune_parameters.set_max_clinical_probability(0.9);
        immune_parameters.set_immune_effect_on_progression_to_clinical(0.4);
        immune_parameters.set_age_mature_immunity(10);
        immune_parameters.set_factor_effect_age_mature_immunity(0.7);
        immune_parameters.set_midpoint(0.3);

        TreatmentImmunityConfig ti;
        ti.enable = true;
        ti.boost_on_treatment = 0.05;
        ti.boost_on_untreated_clinical = 0.02;
        ti.max_extra_boost = 0.3;
        ti.half_life_days = 365.0;
        immune_parameters.set_treatment_immunity(ti);
    }
};

// Test encoding functionality for ImmuneSystemParameters
TEST_F(ImmuneSystemParametersYAMLTest, EncodeImmuneSystemParameters) {
    YAML::Node node = YAML::convert<ImmuneSystemParameters>::encode(immune_parameters);

    // Validate encoding
    EXPECT_EQ(node["b1"].as<double>(), 0.1);
    EXPECT_EQ(node["b2"].as<double>(), 0.2);
    EXPECT_EQ(node["duration_for_naive"].as<int>(), 30);
    EXPECT_EQ(node["duration_for_fully_immune"].as<int>(), 120);
    EXPECT_EQ(node["mean_initial_condition"].as<double>(), 0.5);
    EXPECT_EQ(node["sd_initial_condition"].as<double>(), 0.05);
    EXPECT_EQ(node["immune_inflation_rate"].as<double>(), 1.1);
    EXPECT_EQ(node["min_clinical_probability"].as<double>(), 0.01);
    EXPECT_EQ(node["max_clinical_probability"].as<double>(), 0.9);
    EXPECT_EQ(node["immune_effect_on_progression_to_clinical"].as<double>(), 0.4);
    EXPECT_EQ(node["age_mature_immunity"].as<int>(), 10);
    EXPECT_EQ(node["factor_effect_age_mature_immunity"].as<double>(), 0.7);
    EXPECT_EQ(node["midpoint"].as<double>(), 0.3);

    // Validate treatment immunity config
    EXPECT_TRUE(node["treatment_immunity"]["enable"].as<bool>());
    EXPECT_EQ(node["treatment_immunity"]["boost_on_treatment"].as<double>(), 0.05);
    EXPECT_EQ(node["treatment_immunity"]["boost_on_untreated_clinical"].as<double>(), 0.02);
    EXPECT_EQ(node["treatment_immunity"]["max_extra_boost"].as<double>(), 0.3);
    EXPECT_EQ(node["treatment_immunity"]["half_life_days"].as<double>(), 365.0);
}

// Test decoding functionality for ImmuneSystemParameters
TEST_F(ImmuneSystemParametersYAMLTest, DecodeImmuneSystemParameters) {
    YAML::Node node;
    node["b1"] = 0.1;
    node["b2"] = 0.2;
    node["duration_for_naive"] = 30;
    node["duration_for_fully_immune"] = 120;
    node["mean_initial_condition"] = 0.5;
    node["sd_initial_condition"] = 0.05;
    node["immune_inflation_rate"] = 1.1;
    node["min_clinical_probability"] = 0.01;
    node["max_clinical_probability"] = 0.9;
    node["immune_effect_on_progression_to_clinical"] = 0.4;
    node["age_mature_immunity"] = 10;
    node["factor_effect_age_mature_immunity"] = 0.7;
    node["midpoint"] = 0.3;

    node["treatment_immunity"]["enable"] = true;
    node["treatment_immunity"]["boost_on_treatment"] = 0.05;
    node["treatment_immunity"]["boost_on_untreated_clinical"] = 0.02;
    node["treatment_immunity"]["max_extra_boost"] = 0.3;
    node["treatment_immunity"]["half_life_days"] = 365.0;

    ImmuneSystemParameters decoded_parameters;
    EXPECT_NO_THROW(YAML::convert<ImmuneSystemParameters>::decode(node, decoded_parameters));

    // Validate decoding
    EXPECT_EQ(decoded_parameters.get_b1(), 0.1);
    EXPECT_EQ(decoded_parameters.get_b2(), 0.2);
    EXPECT_EQ(decoded_parameters.get_duration_for_naive(), 30);
    EXPECT_EQ(decoded_parameters.get_duration_for_fully_immune(), 120);
    EXPECT_EQ(decoded_parameters.get_mean_initial_condition(), 0.5);
    EXPECT_EQ(decoded_parameters.get_sd_initial_condition(), 0.05);
    EXPECT_EQ(decoded_parameters.get_immune_inflation_rate(), 1.1);
    EXPECT_EQ(decoded_parameters.get_min_clinical_probability(), 0.01);
    EXPECT_EQ(decoded_parameters.get_max_clinical_probability(), 0.9);
    EXPECT_EQ(decoded_parameters.get_immune_effect_on_progression_to_clinical(), 0.4);
    EXPECT_EQ(decoded_parameters.get_age_mature_immunity(), 10);
    EXPECT_EQ(decoded_parameters.get_factor_effect_age_mature_immunity(), 0.7);
    EXPECT_EQ(decoded_parameters.get_midpoint(), 0.3);

    // Validate treatment immunity config
    EXPECT_TRUE(decoded_parameters.get_treatment_immunity().enable);
    EXPECT_EQ(decoded_parameters.get_treatment_immunity().boost_on_treatment, 0.05);
    EXPECT_EQ(decoded_parameters.get_treatment_immunity().boost_on_untreated_clinical, 0.02);
    EXPECT_EQ(decoded_parameters.get_treatment_immunity().max_extra_boost, 0.3);
    EXPECT_EQ(decoded_parameters.get_treatment_immunity().half_life_days, 365.0);
}

// Test for decoding with missing fields
TEST_F(ImmuneSystemParametersYAMLTest, DecodeImmuneSystemParametersMissingField) {
    YAML::Node node;
    node["b1"] = 0.1;  // Missing other fields

    ImmuneSystemParameters decoded_parameters;
    EXPECT_THROW(YAML::convert<ImmuneSystemParameters>::decode(node, decoded_parameters), std::runtime_error);
}
