#include <gtest/gtest.h>

#include "Configuration/MovementSettings.h"

TEST(MovementSettingsAccessorCoverageTest, CoversDistributionAndMovementAccessors) {
  MovementSettings::MovingLevelDistributionExponential exponential;
  exponential.set_mean(2.0);
  exponential.set_sd(3.0);
  EXPECT_DOUBLE_EQ(exponential.get_mean(), 2.0);
  EXPECT_DOUBLE_EQ(exponential.get_sd(), 3.0);

  MovementSettings::CirculationInfo circulation;
  circulation.set_circulation_percent(12.0);
  circulation.set_relative_probability_that_child_travels_compared_to_adult(0.7);
  circulation.set_relative_probability_for_clinical_to_travel(0.8);
  EXPECT_DOUBLE_EQ(circulation.get_circulation_percent(), 12.0);
  EXPECT_DOUBLE_EQ(circulation.get_relative_probability_that_child_travels_compared_to_adult(),
                   0.7);
  EXPECT_DOUBLE_EQ(circulation.get_relative_probability_for_clinical_to_travel(), 0.8);

  MovementSettings settings;
  settings.set_circulation_info(circulation);
  settings.set_v_moving_level_density({0.2, 0.8});
  settings.set_v_moving_level_value({1.0, 2.0});
  settings.set_length_of_stay_theta(1.5);
  settings.set_length_of_stay_k(2.5);
  EXPECT_EQ(settings.get_v_moving_level_density().size(), 2U);
  EXPECT_EQ(settings.get_v_moving_level_value().size(), 2U);
  EXPECT_DOUBLE_EQ(settings.get_length_of_stay_theta(), 1.5);
  EXPECT_DOUBLE_EQ(settings.get_length_of_stay_k(), 2.5);
}
