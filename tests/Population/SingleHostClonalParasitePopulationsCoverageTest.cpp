#include <gtest/gtest.h>

#include <cmath>

#include "Population/ClonalParasitePopulation.h"

double log10_sum(double log10_a, double log10_b);

TEST(SingleHostClonalParasitePopulationsCoverageTest, Log10SumHandlesZeroDensityOperands) {
  const auto zero = ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY;
  EXPECT_DOUBLE_EQ(log10_sum(zero, 2.0), 2.0);
  EXPECT_DOUBLE_EQ(log10_sum(2.0, zero), 2.0);
}

TEST(SingleHostClonalParasitePopulationsCoverageTest, Log10SumCombinesPositiveDensities) {
  EXPECT_NEAR(log10_sum(1.0, 1.0), std::log10(20.0), 1e-12);
  EXPECT_NEAR(log10_sum(3.0, 1.0), std::log10(1010.0), 1e-12);
}
