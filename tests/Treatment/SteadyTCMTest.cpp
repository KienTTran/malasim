#include <gtest/gtest.h>
#include <memory>

#include "Treatment/SteadyTCM.h"
#include "apps/malasim/MaSimAppInput.h"

class SteadyTCMTest : public ::testing::Test {
protected:
  void SetUp() override {
    tcm = std::make_unique<SteadyTCM>();
    tcm->p_treatment_under_5 = {0.6, 0.7};
    tcm->p_treatment_over_5 = {0.4, 0.5};
  }

  void TearDown() override {
    tcm.reset();
  }

  std::unique_ptr<SteadyTCM> tcm;
};

TEST_F(SteadyTCMTest, MonthlyUpdate) {
  // Store original values for comparison
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double under_5_loc1 = tcm->p_treatment_under_5[1];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];
  const double over_5_loc1 = tcm->p_treatment_over_5[1];

  // Call monthly update
  tcm->monthly_update();

  // Steady should not change the values
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], under_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[1], under_5_loc1);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], over_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[1], over_5_loc1);
}

TEST_F(SteadyTCMTest, MultipleMonthlyUpdates) {
  // Store original values for comparison
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double under_5_loc1 = tcm->p_treatment_under_5[1];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];
  const double over_5_loc1 = tcm->p_treatment_over_5[1];

  // Call monthly update multiple times
  for (int i = 0; i < 12; i++) {
    tcm->monthly_update();
  }

  // Values should remain unchanged after multiple updates
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], under_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[1], under_5_loc1);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], over_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[1], over_5_loc1);
}

TEST_F(SteadyTCMTest, ProbabilityConsistency) {
  // Treatment probabilities should be consistent before and after updates
  const double prob_under_5_loc0 = tcm->get_probability_to_be_treated(0, 3);
  const double prob_over_5_loc1 = tcm->get_probability_to_be_treated(1, 10);
  
  // Call monthly update
  tcm->monthly_update();
  
  // Probabilities should remain the same
  EXPECT_DOUBLE_EQ(tcm->get_probability_to_be_treated(0, 3), prob_under_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->get_probability_to_be_treated(1, 10), prob_over_5_loc1);
}
