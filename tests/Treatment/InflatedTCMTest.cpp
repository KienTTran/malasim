#include <gtest/gtest.h>
#include <memory>

#include "Treatment/InflatedTCM.h"
#include "apps/malasim/MaSimAppInput.h"

class InflatedTCMTest : public ::testing::Test {
protected:
  void SetUp() override {
    tcm = std::make_unique<InflatedTCM>();
    tcm->p_treatment_under_5 = {0.6, 0.7};
    tcm->p_treatment_over_5 = {0.4, 0.5};
    tcm->monthly_inflation_rate = 0.01; // 1% monthly inflation (approx 12% annual)
  }

  void TearDown() override {
    tcm.reset();
  }

  std::unique_ptr<InflatedTCM> tcm;
};

TEST_F(InflatedTCMTest, Initialization) {
  // Test default constructor and initialization
  auto default_tcm = std::make_unique<InflatedTCM>();
  EXPECT_DOUBLE_EQ(default_tcm->monthly_inflation_rate, 0.0);
}

TEST_F(InflatedTCMTest, SingleMonthlyUpdate) {
  // Store original values
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double under_5_loc1 = tcm->p_treatment_under_5[1];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];
  const double over_5_loc1 = tcm->p_treatment_over_5[1];

  // Apply monthly update
  tcm->monthly_update();

  // Values should increase by the monthly inflation rate
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], under_5_loc0 * (1 + tcm->monthly_inflation_rate));
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[1], under_5_loc1 * (1 + tcm->monthly_inflation_rate));
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], over_5_loc0 * (1 + tcm->monthly_inflation_rate));
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[1], over_5_loc1 * (1 + tcm->monthly_inflation_rate));
}

TEST_F(InflatedTCMTest, MultipleMonthlyUpdates) {
  // Store original values
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];

  // Apply multiple monthly updates
  for (int i = 0; i < 3; i++) {
    tcm->monthly_update();
  }

  // Calculate expected values after 3 months
  const double expected_under_5 = under_5_loc0 * std::pow(1 + tcm->monthly_inflation_rate, 3);
  const double expected_over_5 = over_5_loc0 * std::pow(1 + tcm->monthly_inflation_rate, 3);

  EXPECT_NEAR(tcm->p_treatment_under_5[0], expected_under_5, 1e-10);
  EXPECT_NEAR(tcm->p_treatment_over_5[0], expected_over_5, 1e-10);
}

TEST_F(InflatedTCMTest, MaxValueCapping) {
  // Set values close to 1.0 to test capping
  tcm->p_treatment_under_5[0] = 0.99;
  tcm->p_treatment_over_5[0] = 0.98;
  tcm->monthly_inflation_rate = 0.05; // 5% monthly inflation

  // Apply monthly update
  tcm->monthly_update();

  // Values should be capped at 1.0
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], 1.0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], 1.0);
}

TEST_F(InflatedTCMTest, ZeroInflationRate) {
  // Set inflation rate to zero
  tcm->monthly_inflation_rate = 0.0;
  
  // Store original values
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];

  // Apply monthly update
  tcm->monthly_update();

  // Values should remain unchanged with zero inflation
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], under_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], over_5_loc0);
}

TEST_F(InflatedTCMTest, NegativeInflationRate) {
  // Set negative inflation rate (deflation)
  tcm->monthly_inflation_rate = -0.01;
  
  // Store original values
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];

  // Apply monthly update
  tcm->monthly_update();

  // Values should decrease with negative inflation
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], under_5_loc0 * (1 - 0.01));
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], over_5_loc0 * (1 - 0.01));
}
