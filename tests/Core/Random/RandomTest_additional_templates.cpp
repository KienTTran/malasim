#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "RandomTestBase.h"

TEST_F(RandomTest, RandomFlatAndNormalTemplateProduceValuesInExpectedRanges) {
  const auto flat = rng.random_flat(-2.0, 3.0);
  EXPECT_GE(flat, -2.0);
  EXPECT_LT(flat, 3.0);

  const auto integer = rng.random_normal<int>(10, 2.0);
  EXPECT_GE(integer, -100);
  EXPECT_LE(integer, 100);

  const auto real = rng.random_normal<double>(10.0, 2.0);
  EXPECT_TRUE(std::isfinite(real));
}

TEST_F(RandomTest, RandomNormalTemplateRejectsInvalidDeviation) {
  EXPECT_THROW(rng.random_normal<int>(0, 0.0), std::invalid_argument);
  EXPECT_THROW(rng.random_normal<double>(0.0, -1.0), std::invalid_argument);
}
