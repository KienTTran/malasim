#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Configuration/EpidemiologicalParameters.h"

namespace {
YAML::Node minimal_epidemiological_node() {
  YAML::Node node;
  node["number_of_tracking_days"] = 10;
  node["days_to_clinical_under_five"] = 2;
  node["days_to_clinical_over_five"] = 3;
  node["days_mature_gametocyte_under_five"] = 4;
  node["days_mature_gametocyte_over_five"] = 5;
  node["p_compliance"] = 0.8;
  node["min_dosing_days"] = 1;
  node["relative_biting_info"]["max_relative_biting_value"] = 10;
  node["relative_biting_info"]["min_relative_biting_value"] = 1.0;
  node["relative_biting_info"]["number_of_biting_levels"] = 2;
  node["relative_biting_info"]["biting_level_distribution"]["distribution"] = "Gamma";
  node["relative_biting_info"]["biting_level_distribution"]["Gamma"]["mean"] = 2.0;
  node["relative_biting_info"]["biting_level_distribution"]["Gamma"]["sd"] = 1.0;
  node["relative_biting_info"]["biting_level_distribution"]["Exponential"]["scale"] = 1.0;
  node["gametocyte_level_under_artemisinin_action"] = 0.2;
  node["gametocyte_level_full"] = 1.0;
  node["relative_infectivity"]["sigma"] = 3.0;
  node["relative_infectivity"]["ro"] = 0.1;
  node["relative_infectivity"]["blood_meal_volume"] = 2.0;
  node["p_relapse"] = 0.1;
  node["relapse_duration"] = 20;
  node["relapse_rate"] = 0.2;
  node["update_frequency"] = 1;
  node["allow_new_coinfection_to_cause_symptoms"] = true;
  node["tf_window_size"] = 5;
  node["fraction_mosquitoes_interrupted_feeding"] = 0.1;
  node["inflation_factor"] = 1.0;
  node["using_age_dependent_biting_level"] = false;
  node["using_variable_probability_infectious_bites_cause_infection"] = false;
  return node;
}
}  // namespace

TEST(EpidemiologicalNestedConversionCoverageTest, EncodesEnabledAgeSeekingConfiguration) {
  EpidemiologicalParameters parameters;
  EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment age;
  age.set_enabled(true);
  age.set_type("power");
  age.set_ages({0, 5, 10});
  EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::PowerConfig power;
  power.base = 0.75;
  power.exponent_source = "index";
  age.set_power(power);
  parameters.set_age_based_probability_of_seeking_treatment(age);

  const auto node = YAML::convert<EpidemiologicalParameters>::encode(parameters);
  EXPECT_EQ(node["age_based_probability_of_seeking_treatment"]["type"].as<std::string>(), "power");
  EXPECT_DOUBLE_EQ(
      node["age_based_probability_of_seeking_treatment"]["power"]["base"].as<double>(), 0.75);
  EXPECT_EQ(node["age_based_probability_of_seeking_treatment"]["ages"].as<std::vector<int>>(),
            (std::vector<int>{0, 5, 10}));
}

TEST(EpidemiologicalNestedConversionCoverageTest, DecodesLegacyCoinfectionAndOptionalAgeSeeking) {
  auto node = minimal_epidemiological_node();
  node["age_based_probability_of_seeking_treatment"]["type"] = "power";
  node["age_based_probability_of_seeking_treatment"]["ages"] = std::vector<int>{0, 10};

  EpidemiologicalParameters decoded;
  ASSERT_TRUE(YAML::convert<EpidemiologicalParameters>::decode(node, decoded));
  EXPECT_TRUE(decoded.get_allow_new_coinfection_to_cause_symptoms().get_enable());
  EXPECT_DOUBLE_EQ(decoded.get_allow_new_coinfection_to_cause_symptoms().get_probability(), 1.0);
  EXPECT_TRUE(decoded.get_age_based_probability_of_seeking_treatment().is_enabled());
  EXPECT_EQ(decoded.get_age_based_probability_of_seeking_treatment().get_ages(),
            (std::vector<int>{0, 10}));
}

TEST(EpidemiologicalNestedConversionCoverageTest, RejectsMissingNestedDistributionFields) {
  EpidemiologicalParameters::BitingLevelDistributionGamma gamma;
  YAML::Node gamma_node;
  gamma_node["mean"] = 1.0;
  EXPECT_THROW(YAML::convert<EpidemiologicalParameters::BitingLevelDistributionGamma>::decode(
                   gamma_node, gamma),
               std::runtime_error);

  EpidemiologicalParameters::BitingLevelDistributionExponential exponential;
  EXPECT_THROW(
      YAML::convert<EpidemiologicalParameters::BitingLevelDistributionExponential>::decode(
          YAML::Node{}, exponential),
      std::runtime_error);

  EpidemiologicalParameters::RelativeInfectivity infectivity;
  YAML::Node infectivity_node;
  infectivity_node["sigma"] = 1.0;
  EXPECT_THROW(YAML::convert<EpidemiologicalParameters::RelativeInfectivity>::decode(
                   infectivity_node, infectivity),
               std::runtime_error);
}
