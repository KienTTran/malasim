#include <gtest/gtest.h>
#include <memory>

#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Treatment/LinearTCM.h"
#include "Utils/Cli.h"

class LinearTCMTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    tcm = std::make_unique<LinearTCM>();
    tcm->starting_time = 0;
    tcm->end_time = 1825;  // 5 years = 1825 days
    
    // Initial treatment probabilities
    tcm->p_treatment_under_5 = {0.6, 0.7};
    tcm->p_treatment_over_5 = {0.4, 0.5};
    
    // Target treatment probabilities
    tcm->p_treatment_under_5_to = {0.8, 0.9};
    tcm->p_treatment_over_5_to = {0.6, 0.7};
    
    // Calculate rate of change (should be called after setting the values)
    tcm->update_rate_of_change();
  }

  void TearDown() override {
    tcm.reset();
  }

  std::unique_ptr<LinearTCM> tcm;
};

TEST_F(LinearTCMTest, UpdateRateOfChange) {
  // Clear existing rates
  tcm->rate_of_change_under_5.clear();
  tcm->rate_of_change_over_5.clear();
  
  // Recalculate rates
  tcm->update_rate_of_change();
  
  // Expected rates (daily increase multiplied by 30 for monthly update)
  const double expected_rate_under_5_loc0 = 30.0 * (0.8 - 0.6) / 1825.0;
  const double expected_rate_under_5_loc1 = 30.0 * (0.9 - 0.7) / 1825.0;
  const double expected_rate_over_5_loc0 = 30.0 * (0.6 - 0.4) / 1825.0;
  const double expected_rate_over_5_loc1 = 30.0 * (0.7 - 0.5) / 1825.0;
  
  ASSERT_EQ(tcm->rate_of_change_under_5.size(), 2);
  ASSERT_EQ(tcm->rate_of_change_over_5.size(), 2);
  
  EXPECT_NEAR(tcm->rate_of_change_under_5[0], expected_rate_under_5_loc0, 1e-10);
  EXPECT_NEAR(tcm->rate_of_change_under_5[1], expected_rate_under_5_loc1, 1e-10);
  EXPECT_NEAR(tcm->rate_of_change_over_5[0], expected_rate_over_5_loc0, 1e-10);
  EXPECT_NEAR(tcm->rate_of_change_over_5[1], expected_rate_over_5_loc1, 1e-10);
}

TEST_F(LinearTCMTest, MonthlyUpdateDuringValidPeriod) {
  // Store original values
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double under_5_loc1 = tcm->p_treatment_under_5[1];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];
  const double over_5_loc1 = tcm->p_treatment_over_5[1];
  
  // Ensure we're within the valid period
  Model::get_scheduler()->set_current_time(500);  // Within the 0-1825 range
  
  // Apply monthly update
  tcm->monthly_update();
  
  // Values should increase by the calculated rates
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], under_5_loc0 + tcm->rate_of_change_under_5[0]);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[1], under_5_loc1 + tcm->rate_of_change_under_5[1]);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], over_5_loc0 + tcm->rate_of_change_over_5[0]);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[1], over_5_loc1 + tcm->rate_of_change_over_5[1]);
}

TEST_F(LinearTCMTest, MonthlyUpdateAfterEndTime) {
  // Store original values
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double under_5_loc1 = tcm->p_treatment_under_5[1];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];
  const double over_5_loc1 = tcm->p_treatment_over_5[1];
  
  // Set time after the end time
  Model::get_scheduler()->set_current_time(2000);  // After the 1825 end time
  
  // Apply monthly update
  tcm->monthly_update();
  
  // Values should not change after end time
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[0], under_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_under_5[1], under_5_loc1);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[0], over_5_loc0);
  EXPECT_DOUBLE_EQ(tcm->p_treatment_over_5[1], over_5_loc1);
}

TEST_F(LinearTCMTest, MultipleMonthlyUpdates) {
  // Store original values
  const double under_5_loc0 = tcm->p_treatment_under_5[0];
  const double over_5_loc0 = tcm->p_treatment_over_5[0];
  
  // Set time at the beginning
  Model::get_scheduler()->set_current_time(0);
  
  // Apply multiple monthly updates
  const int num_updates = 3;
  for (int i = 0; i < num_updates; i++) {
    tcm->monthly_update();
  }
  
  // Expected values after 3 updates
  const double expected_under_5 = under_5_loc0 + (num_updates * tcm->rate_of_change_under_5[0]);
  const double expected_over_5 = over_5_loc0 + (num_updates * tcm->rate_of_change_over_5[0]);
  
  EXPECT_NEAR(tcm->p_treatment_under_5[0], expected_under_5, 1e-10);
  EXPECT_NEAR(tcm->p_treatment_over_5[0], expected_over_5, 1e-10);
}

TEST_F(LinearTCMTest, CompleteTransition) {
  // Calculate exactly how many monthly updates we would need to reach the final values
  // Set time at the beginning
  Model::get_scheduler()->set_current_time(0);
  
  // Calculate number of months in the transition period (1825 days / 30 days per month ≈ 61 months)
  const int num_months = 61;
  
  // Apply updates for the entire period
  for (int i = 0; i < num_months; i++) {
    tcm->monthly_update();
  }
  
  // Values should approach the target values
  // We use near because of potential floating-point rounding errors
  EXPECT_NEAR(tcm->p_treatment_under_5[0], tcm->p_treatment_under_5_to[0], 0.01);
  EXPECT_NEAR(tcm->p_treatment_under_5[1], tcm->p_treatment_under_5_to[1], 0.01);
  EXPECT_NEAR(tcm->p_treatment_over_5[0], tcm->p_treatment_over_5_to[0], 0.01);
  EXPECT_NEAR(tcm->p_treatment_over_5[1], tcm->p_treatment_over_5_to[1], 0.01);
}
