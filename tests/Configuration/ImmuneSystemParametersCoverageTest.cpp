#include <gtest/gtest.h>

#include "Configuration/ImmuneSystemParameters.h"

TEST(ImmuneSystemParametersCoverageTest, ValidatesProbabilityAndDurationBounds) {
  ImmuneSystemParameters parameters;
  EXPECT_THROW(parameters.set_min_clinical_probability(-0.01), std::invalid_argument);
  EXPECT_THROW(parameters.set_max_clinical_probability(1.01), std::invalid_argument);
  EXPECT_THROW(parameters.set_duration_for_naive(-1), std::invalid_argument);
  EXPECT_THROW(parameters.set_duration_for_fully_immune(-1), std::invalid_argument);
  EXPECT_THROW(parameters.set_age_mature_immunity(-1), std::invalid_argument);
}

TEST(ImmuneSystemParametersCoverageTest, ProcessesZeroAndNonzeroInitialConditionDeviation) {
  ImmuneSystemParameters zero_sd;
  zero_sd.set_sd_initial_condition(0.0);
  ASSERT_NO_THROW(zero_sd.process_config_with_parasite_density(5.0, 2.0));
  EXPECT_DOUBLE_EQ(zero_sd.get_alpha_immune(), zero_sd.get_mean_initial_condition());
  EXPECT_DOUBLE_EQ(zero_sd.get_beta_immune(), 0.0);
  EXPECT_EQ(zero_sd.get_acquire_rate_by_age().size(), 81U);

  ImmuneSystemParameters nonzero_sd;
  nonzero_sd.set_sd_initial_condition(0.1);
  ASSERT_NO_THROW(nonzero_sd.process_config_with_parasite_density(5.0, 2.0));
  EXPECT_GT(nonzero_sd.get_alpha_immune(), 0.0);
  EXPECT_GT(nonzero_sd.get_beta_immune(), 0.0);
  EXPECT_EQ(nonzero_sd.get_acquire_rate_by_age_one_day_factor().size(), 81U);
}
