#include <gtest/gtest.h>
#include <memory>
#include <map>

#include "Treatment/Strategies/MFTRebalancingStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "MDC/ModelDataCollector.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class MFTRebalancingStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<MFTRebalancingStrategy>();
    strategy->id = 1;
    strategy->name = "TestMFTRebalancingStrategy";
    strategy->update_duration_after_rebalancing = 90;  // 90 days between updates
    strategy->delay_until_actual_trigger = 30;        // 30 days delay before applying new distribution
    
    // Initialize next_distribution vector
    strategy->next_distribution = {0.0, 0.0};
    
    // Create therapies
    therapy1 = std::make_unique<Therapy>();
    therapy1->set_id(101);
    therapy1->set_name("Therapy 1");
    
    therapy2 = std::make_unique<Therapy>();
    therapy2->set_id(102);
    therapy2->set_name("Therapy 2");
    
    // Create a test person
    person = std::make_unique<Person>();
    
    // Prepare model data collector with TF values
    mdc = Model::get_mdc();
  }

  void TearDown() override {
    person.reset();
    therapy2.reset();
    therapy1.reset();
    strategy.reset();
    test_fixtures::cleanup_test_files();
  }
  
  // Helper function to set TF values in ModelDataCollector
  void set_tf_value_for_therapy(int therapy_id, double tf_value) {
    // Create a map of therapy IDs to TF values if it doesn't exist
    if (mdc->current_tf_by_therapy().empty()) {
      mdc->set_current_tf_by_therapy(std::vector<double>());
    }
    
    // Set the TF value for the specified therapy
    mdc->current_tf_by_therapy()[therapy_id] = tf_value;
  }

  std::unique_ptr<MFTRebalancingStrategy> strategy;
  std::unique_ptr<Therapy> therapy1;
  std::unique_ptr<Therapy> therapy2;
  std::unique_ptr<Person> person;
  ModelDataCollector* mdc;
};

TEST_F(MFTRebalancingStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestMFTRebalancingStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::MFTRebalancing);
  EXPECT_TRUE(strategy->therapy_list.empty());
  EXPECT_TRUE(strategy->distribution.empty());
  EXPECT_EQ(strategy->update_duration_after_rebalancing, 90);
  EXPECT_EQ(strategy->delay_until_actual_trigger, 30);
  EXPECT_EQ(strategy->latest_adjust_distribution_time, 0);
  EXPECT_EQ(strategy->next_update_time, 0);
  ASSERT_EQ(strategy->next_distribution.size(), 2);
  EXPECT_DOUBLE_EQ(strategy->next_distribution[0], 0.0);
  EXPECT_DOUBLE_EQ(strategy->next_distribution[1], 0.0);
}

TEST_F(MFTRebalancingStrategyTest, AddTherapyAndDistribution) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set initial distribution
  strategy->distribution = {0.6, 0.4};
  
  // Verify therapies and distribution were added
  ASSERT_EQ(strategy->therapy_list.size(), 2);
  EXPECT_EQ(strategy->therapy_list[0], therapy1.get());
  EXPECT_EQ(strategy->therapy_list[1], therapy2.get());
  
  ASSERT_EQ(strategy->distribution.size(), 2);
  EXPECT_DOUBLE_EQ(strategy->distribution[0], 0.6);
  EXPECT_DOUBLE_EQ(strategy->distribution[1], 0.4);
}

TEST_F(MFTRebalancingStrategyTest, GetTherapy) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set distribution
  strategy->distribution = {0.6, 0.4};
  
  // Test that the therapy selection follows the parent MFTStrategy behavior
  Therapy* selected_therapy = strategy->get_therapy(person.get());
  EXPECT_TRUE(selected_therapy == therapy1.get() || selected_therapy == therapy2.get());
}

TEST_F(MFTRebalancingStrategyTest, ToString) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set distribution
  strategy->distribution = {0.6, 0.4};
  
  // Test the string representation
  std::string expected = "1-TestMFTRebalancingStrategy-101,102-0.6,0.4-90";
  EXPECT_EQ(strategy->to_string(), expected);
}

TEST_F(MFTRebalancingStrategyTest, UpdateEndOfTimeStepAtUpdateTime) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set distribution and next_distribution
  strategy->distribution = {0.6, 0.4};
  strategy->next_distribution = {0.3, 0.7};
  
  // Set current time to match next_update_time
  const int current_time = 100;
  Model::get_scheduler()->set_current_time(current_time);
  strategy->next_update_time = current_time;
  
  // Set TF values for therapies
  set_tf_value_for_therapy(therapy1->get_id(), 0.2);  // 20% failure rate
  set_tf_value_for_therapy(therapy2->get_id(), 0.1);  // 10% failure rate
  
  // Call update_end_of_time_step
  strategy->update_end_of_time_step();
  
  // Verify that next_distribution has been calculated based on inverse of TF rates
  // For therapy1: 1/0.2 = 5
  // For therapy2: 1/0.1 = 10
  // Total: 15
  // Normalized: therapy1 = 5/15 = 0.333..., therapy2 = 10/15 = 0.666...
  ASSERT_EQ(strategy->next_distribution.size(), 2);
  EXPECT_NEAR(strategy->next_distribution[0], 0.333, 0.001);
  EXPECT_NEAR(strategy->next_distribution[1], 0.667, 0.001);
  
  // Verify that latest_adjust_distribution_time has been set
  EXPECT_EQ(strategy->latest_adjust_distribution_time, current_time + strategy->delay_until_actual_trigger);
}

TEST_F(MFTRebalancingStrategyTest, UpdateEndOfTimeStepWithLowTF) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set distribution and next_distribution
  strategy->distribution = {0.5, 0.5};
  strategy->next_distribution = {0.0, 0.0};
  
  // Set current time to match next_update_time
  const int current_time = 100;
  Model::get_scheduler()->set_current_time(current_time);
  strategy->next_update_time = current_time;
  
  // Set very low TF value (below 0.05 threshold) for therapy1
  set_tf_value_for_therapy(therapy1->get_id(), 0.03);  // 3% failure rate (below threshold)
  set_tf_value_for_therapy(therapy2->get_id(), 0.1);   // 10% failure rate
  
  // Call update_end_of_time_step
  strategy->update_end_of_time_step();
  
  // Verify that next_distribution has been calculated with the minimum TF rate (0.05) for therapy1
  // For therapy1: 1/0.05 = 20 (using minimum threshold since actual TF is too low)
  // For therapy2: 1/0.1 = 10
  // Total: 30
  // Normalized: therapy1 = 20/30 = 0.667, therapy2 = 10/30 = 0.333
  ASSERT_EQ(strategy->next_distribution.size(), 2);
  EXPECT_NEAR(strategy->next_distribution[0], 0.667, 0.001);
  EXPECT_NEAR(strategy->next_distribution[1], 0.333, 0.001);
}

TEST_F(MFTRebalancingStrategyTest, UpdateEndOfTimeStepAtAdjustTime) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set distribution and next_distribution
  strategy->distribution = {0.6, 0.4};
  strategy->next_distribution = {0.3, 0.7};
  
  // Set current time to match latest_adjust_distribution_time
  const int current_time = 150;
  Model::get_scheduler()->set_current_time(current_time);
  strategy->latest_adjust_distribution_time = current_time;
  
  // Call update_end_of_time_step
  strategy->update_end_of_time_step();
  
  // Verify that distribution has been updated with next_distribution values
  ASSERT_EQ(strategy->distribution.size(), 2);
  EXPECT_DOUBLE_EQ(strategy->distribution[0], 0.3);
  EXPECT_DOUBLE_EQ(strategy->distribution[1], 0.7);
  
  // Verify that next_update_time has been set
  EXPECT_EQ(strategy->next_update_time, current_time + strategy->update_duration_after_rebalancing);
}

TEST_F(MFTRebalancingStrategyTest, AdjustStartedTimePoint) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set distribution
  strategy->distribution = {0.6, 0.4};
  
  // Set some values to be reset
  strategy->latest_adjust_distribution_time = 100;
  
  // Set current time
  const int current_time = 200;
  Model::get_scheduler()->set_current_time(current_time);
  
  // Adjust start time point
  strategy->adjust_started_time_point(current_time);
  
  // Verify that next_update_time is set correctly
  EXPECT_EQ(strategy->next_update_time, current_time + strategy->update_duration_after_rebalancing);
  
  // Verify that latest_adjust_distribution_time is reset to -1
  EXPECT_EQ(strategy->latest_adjust_distribution_time, -1);
}
