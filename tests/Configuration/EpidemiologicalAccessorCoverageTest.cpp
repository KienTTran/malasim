#include <gtest/gtest.h>

#include "Configuration/EpidemiologicalParameters.h"

TEST(EpidemiologicalAccessorCoverageTest, CoversNestedDistributionAndInfectivityAccessors) {
  EpidemiologicalParameters::BitingLevelDistributionGamma gamma;
  gamma.set_mean(4.0);
  gamma.set_sd(2.0);
  EXPECT_DOUBLE_EQ(gamma.get_mean(), 4.0);
  EXPECT_DOUBLE_EQ(gamma.get_sd(), 2.0);

  EpidemiologicalParameters::BitingLevelDistributionExponential exponential;
  exponential.set_scale(0.25);
  EXPECT_DOUBLE_EQ(exponential.get_scale(), 0.25);

  EpidemiologicalParameters::BitingLevelDistribution distribution;
  distribution.set_distribution("gamma");
  distribution.set_gamma(gamma);
  distribution.set_exponential(exponential);
  EXPECT_EQ(distribution.get_distribution(), "gamma");
  EXPECT_DOUBLE_EQ(distribution.get_gamma().get_mean(), 4.0);
  EXPECT_DOUBLE_EQ(distribution.get_exponential().get_scale(), 0.25);

  EpidemiologicalParameters::RelativeBitingInfo biting;
  biting.set_max_relative_biting_value(40);
  biting.set_min_relative_biting_value(0.5);
  biting.set_number_of_biting_levels(12);
  biting.set_biting_level_distribution(distribution);
  biting.set_scale(0.3);
  biting.set_mean(2.0);
  biting.set_sd(1.0);
  EXPECT_EQ(biting.get_max_relative_biting_value(), 40);
  EXPECT_DOUBLE_EQ(biting.get_min_relative_biting_value(), 0.5);
  EXPECT_EQ(biting.get_number_of_biting_levels(), 12);
  EXPECT_DOUBLE_EQ(biting.get_scale(), 0.3);
  EXPECT_DOUBLE_EQ(biting.get_mean(), 2.0);
  EXPECT_DOUBLE_EQ(biting.get_sd(), 1.0);

  EpidemiologicalParameters::RelativeInfectivity infectivity;
  infectivity.set_sigma(2.0);
  infectivity.set_ro_star(0.01);
  infectivity.set_blood_meal_volume(2.5);
  EXPECT_DOUBLE_EQ(infectivity.get_sigma(), 2.0);
  EXPECT_DOUBLE_EQ(infectivity.get_ro_star(), 0.01);
  EXPECT_DOUBLE_EQ(infectivity.get_blood_meal_volume(), 2.5);
}

TEST(EpidemiologicalAccessorCoverageTest, CoversParameterSettersAndAgeSeekingValidation) {
  EpidemiologicalParameters parameters;
  parameters.set_number_of_tracking_days(9);
  parameters.set_days_to_clinical_under_five(3);
  parameters.set_days_to_clinical_over_five(4);
  parameters.set_days_mature_gametocyte_under_five(5);
  parameters.set_days_mature_gametocyte_over_five(6);
  parameters.set_p_compliance(0.7);
  parameters.set_min_dosing_days(2);
  parameters.set_relative_biting_info(EpidemiologicalParameters::RelativeBitingInfo());
  parameters.set_gametocyte_level_under_artemisinin_action(0.2);
  parameters.set_gametocyte_level_full(0.9);
  parameters.set_relative_infectivity(EpidemiologicalParameters::RelativeInfectivity());
  parameters.set_p_relapse(0.1);
  parameters.set_relapse_duration(20);
  parameters.set_relapse_rate(0.05);
  parameters.set_update_frequency(2);
  EpidemiologicalParameters::AllowNewCoinfectionToCauseSymptoms coinfection;
  coinfection.set_enable(true);
  coinfection.set_probability(0.6);
  parameters.set_allow_new_coinfection_to_cause_symptoms(coinfection);
  parameters.set_tf_window_size(60);
  parameters.set_fraction_mosquitoes_interrupted_feeding(0.25);
  parameters.set_inflation_factor(1.2);
  parameters.set_using_age_dependent_biting_level(true);
  parameters.set_using_variable_probability_infectious_bites_cause_infection(true);

  EXPECT_EQ(parameters.get_number_of_tracking_days(), 9);
  EXPECT_EQ(parameters.get_days_to_clinical_under_five(), 3);
  EXPECT_EQ(parameters.get_days_to_clinical_over_five(), 4);
  EXPECT_EQ(parameters.get_days_mature_gametocyte_under_five(), 5);
  EXPECT_EQ(parameters.get_days_mature_gametocyte_over_five(), 6);
  EXPECT_DOUBLE_EQ(parameters.get_p_compliance(), 0.7);
  EXPECT_EQ(parameters.get_min_dosing_days(), 2);
  EXPECT_DOUBLE_EQ(parameters.get_gametocyte_level_full(), 0.9);
  EXPECT_DOUBLE_EQ(parameters.get_p_relapse(), 0.1);
  EXPECT_EQ(parameters.get_relapse_duration(), 20);
  EXPECT_DOUBLE_EQ(parameters.get_relapse_rate(), 0.05);
  EXPECT_EQ(parameters.get_update_frequency(), 2);
  EXPECT_TRUE(parameters.get_allow_new_coinfection_to_cause_symptoms().get_enable());
  EXPECT_DOUBLE_EQ(parameters.get_allow_new_coinfection_to_cause_symptoms().get_probability(), 0.6);
  EXPECT_EQ(parameters.get_tf_window_size(), 60);
  EXPECT_DOUBLE_EQ(parameters.get_fraction_mosquitoes_interrupted_feeding(), 0.25);
  EXPECT_DOUBLE_EQ(parameters.get_inflation_factor(), 1.2);
  EXPECT_TRUE(parameters.get_using_age_dependent_biting_level());
  EXPECT_TRUE(parameters.get_using_variable_probability_infectious_bites_cause_infection());

  auto seeking = parameters.get_age_based_probability_of_seeking_treatment();
  seeking.set_enabled(true);
  seeking.set_type("power");
  seeking.set_ages({0, 5, 10});
  EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::PowerConfig power;
  power.base = 0.8;
  power.exponent_source = "index";
  seeking.set_power(power);
  seeking.validate();
  EXPECT_TRUE(seeking.is_enabled());
  EXPECT_EQ(seeking.get_type(), "power");
  EXPECT_EQ(seeking.get_ages(), std::vector<int>({0, 5, 10}));
  EXPECT_DOUBLE_EQ(seeking.get_power().base, 0.8);
  seeking.set_type("unsupported");
  EXPECT_THROW(seeking.validate(), std::runtime_error);
}
