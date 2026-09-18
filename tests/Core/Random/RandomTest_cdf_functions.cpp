// RandomTest_cdf_functions.cpp

#include <gsl/gsl_cdf.h>
#include <gtest/gtest.h>

#include "RandomTestBase.h"

double approx_norm_cdf(double value);

TEST_F(RandomTest, CdfGammaDistribution_ValidParameters) {
  double value = 2.0;
  double alpha = 3.0;
  double beta = 2.0;
  double expected = gsl_cdf_gamma_P(value, alpha, beta);
  double result = rng.cdf_gamma_distribution(value, alpha, beta);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(RandomTest, CdfGammaDistribution_BetaZero) {
  double value = 5.0;
  double alpha = 2.0;
  double beta = 0.0;
  double expected = 1.0;
  double result = rng.cdf_gamma_distribution(value, alpha, beta);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(RandomTest, CdfGammaDistribution_InvalidAlpha) {
  double value = 1.0;
  double alpha = -1.0;  // Invalid
  double beta = 2.0;
  EXPECT_THROW(rng.cdf_gamma_distribution(value, alpha, beta),
               std::invalid_argument);
}

TEST_F(RandomTest, CdfGammaDistribution_InvalidBeta) {
  double value = 1.0;
  double alpha = 2.0;
  double beta = -0.5;  // Invalid
  EXPECT_THROW(rng.cdf_gamma_distribution(value, alpha, beta),
               std::invalid_argument);
}

// Tests for cdf_gamma_distribution_inverse

TEST_F(RandomTest, CdfGammaDistributionInverse_ValidProbability) {
  double probability = 0.5;
  double alpha = 3.0;
  double beta = 2.0;
  double expected = gsl_cdf_gamma_Pinv(probability, alpha, beta);
  double result = rng.cdf_gamma_distribution_inverse(probability, alpha, beta);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(RandomTest, CdfGammaDistributionInverse_ProbabilityZero) {
  double probability = 0.0;
  double alpha = 2.0;
  double beta = 1.0;
  double expected = 0.0;  // GSL's gsl_cdf_gamma_Pinv(0, ...) should return 0
  double result = rng.cdf_gamma_distribution_inverse(probability, alpha, beta);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(RandomTest, CdfGammaDistributionInverse_ProbabilityOne) {
  double probability = 1.0;
  double alpha = 2.0;
  double beta = 1.0;
  double expected =
      INFINITY;  // GSL's gsl_cdf_gamma_Pinv(1, ...) should return +inf
  double result = rng.cdf_gamma_distribution_inverse(probability, alpha, beta);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(RandomTest, CdfGammaDistributionInverse_InvalidProbabilityLow) {
  double probability = -0.1;  // Invalid
  double alpha = 2.0;
  double beta = 1.0;
  EXPECT_THROW(rng.cdf_gamma_distribution_inverse(probability, alpha, beta),
               std::invalid_argument);
}

TEST_F(RandomTest, CdfGammaDistributionInverse_InvalidProbabilityHigh) {
  double probability = 1.5;  // Invalid
  double alpha = 2.0;
  double beta = 1.0;
  EXPECT_THROW(rng.cdf_gamma_distribution_inverse(probability, alpha, beta),
               std::invalid_argument);
}

// Tests for cdf_standard_normal_distribution

TEST_F(RandomTest, CdfStandardNormalDistribution_ValidValue) {
  double value = 1.0;
  double expected = gsl_cdf_ugaussian_P(value);
  double result = rng.cdf_standard_normal_distribution(value);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(RandomTest, CdfStandardNormalDistribution_ExtremePositive) {
  double value = 10.0;
  double expected = gsl_cdf_ugaussian_P(value);
  double result = rng.cdf_standard_normal_distribution(value);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(RandomTest, CdfStandardNormalDistribution_ExtremeNegative) {
  double value = -10.0;
  double expected = gsl_cdf_ugaussian_P(value);
  double result = rng.cdf_standard_normal_distribution(value);
  EXPECT_DOUBLE_EQ(result, expected);
}

TEST(RandomCoverageTest, NormalCdfApproximationHandlesTailsAndSigns) {
  EXPECT_DOUBLE_EQ(approx_norm_cdf(7.0), 1.0);
  EXPECT_DOUBLE_EQ(approx_norm_cdf(-7.0), 0.0);
  EXPECT_NEAR(approx_norm_cdf(0.0), 0.5, 1e-6);
  EXPECT_NEAR(approx_norm_cdf(-1.0), 1.0 - approx_norm_cdf(1.0), 1e-6);
}
