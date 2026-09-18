#include <gtest/gtest.h>

#include "Configuration/EpidemiologicalParameters.h"

using AgeSeeking = EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment;

TEST(EpidemiologicalAgeSeekingValidationTest, RejectsInvalidEnabledConfigurations) {
  AgeSeeking config;
  config.set_enabled(true);
  EXPECT_THROW(config.validate(), std::runtime_error);

  config.set_type("linear");
  EXPECT_THROW(config.validate(), std::runtime_error);

  config.set_type("power");
  AgeSeeking::PowerConfig power;
  power.exponent_source = "age";
  config.set_power(power);
  config.set_ages({0, 5});
  EXPECT_THROW(config.validate(), std::runtime_error);

  power.exponent_source = "index";
  config.set_power(power);
  config.set_ages({});
  EXPECT_THROW(config.validate(), std::runtime_error);

  config.set_ages({5, 0});
  EXPECT_THROW(config.validate(), std::runtime_error);
  config.set_ages({0, 5, 5});
  EXPECT_THROW(config.validate(), std::runtime_error);
  config.set_ages({-1, 5});
  EXPECT_THROW(config.validate(), std::runtime_error);

  config.set_ages({2, 5});
  EXPECT_NO_THROW(config.validate());
  power.base = -0.1;
  config.set_power(power);
  EXPECT_THROW(config.validate(), std::runtime_error);
}

TEST(EpidemiologicalAgeSeekingValidationTest, HandlesEvaluationFallbacksAndAgeClamping) {
  AgeSeeking config;
  EXPECT_DOUBLE_EQ(config.evaluate_for_age(-10), 1.0);

  config.set_enabled(true);
  config.set_type("power");
  AgeSeeking::PowerConfig power;
  power.base = 0.5;
  power.exponent_source = "index";
  config.set_power(power);
  config.set_ages({0, 10});
  EXPECT_DOUBLE_EQ(config.evaluate_for_age(-10), 1.0);
  EXPECT_NEAR(config.evaluate_for_age(20), 0.5, 1e-12);

  config.set_ages({});
  EXPECT_DOUBLE_EQ(config.evaluate_for_age(20), 1.0);
  config.set_ages({0});
  power.exponent_source = "age";
  config.set_power(power);
  EXPECT_DOUBLE_EQ(config.evaluate_for_age(20), 1.0);
  config.set_type("unknown");
  EXPECT_DOUBLE_EQ(config.evaluate_for_age(20), 1.0);
}
