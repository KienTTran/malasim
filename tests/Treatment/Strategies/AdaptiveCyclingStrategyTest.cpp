#include <gtest/gtest.h>
#include <memory>
#include <map>

#include "Treatment/Strategies/AdaptiveCyclingStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "MDC/ModelDataCollector.h"
#include "Utils/Cli.h"

class AdaptiveCyclingStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<AdaptiveCyclingStrategy>();
    strategy->id = 1;
    strategy->name = "TestAdaptiveCyclingStrategy";
    strategy->trigger_value = 0.15;
    strategy->delay_until_actual_trigger = 30; // 30 days delay
    strategy->turn_off_days = 90; // 90 days minimum between switches
    
    // Create therapies
    therapy1 = std::make_unique<Therapy>();
    therapy1->set_id(101);
    therapy1->set_name("Therapy 1");
    
    therapy2 = std::make_unique<Therapy>();
    therapy2->set_id(102);
    therapy2->set_name("Therapy 2");
    
    therapy3 = std::make_unique<Therapy>();
    therapy3->set_id(103);
    therapy3->set_name("Therapy 3");
    
    // Create a test person
    person = std::make_unique<Person>();
    
    // Prepare model data collector with TF values
    mdc = Model::get_mdc();
  }

  void TearDown() override {
    person.reset();
    therapy3.reset();
    therapy2.reset();
    therapy1.reset();
    strategy.reset();
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

  std::unique_ptr<AdaptiveCyclingStrategy> strategy;
  std::unique_ptr<Therapy> therapy1;
  std::unique_ptr<Therapy> therapy2;
  std::unique_ptr<Therapy> therapy3;
  std::unique_ptr<Person> person;
  ModelDataCollector* mdc;
};

TEST_F(AdaptiveCyclingStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestAdaptiveCyclingStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::AdaptiveCycling);
  EXPECT_TRUE(strategy->therapy_list.empty());
  EXPECT_EQ(strategy->index, 0);
  EXPECT_DOUBLE_EQ(strategy->trigger_value, 0.15);
  EXPECT_EQ(strategy->delay_until_actual_trigger, 30);
  EXPECT_EQ(strategy->turn_off_days, 90);
  EXPECT_EQ(strategy->latest_switch_time, 0);
}

TEST_F(AdaptiveCyclingStrategyTest, AddTherapy) {
  // Add first therapy
  strategy->add_therapy(therapy1.get());
  
  ASSERT_EQ(strategy->therapy_list.size(), 1);
  EXPECT_EQ(strategy->therapy_list[0], therapy1.get());
  
  // Add second therapy
  strategy->add_therapy(therapy2.get());
  
  ASSERT_EQ(strategy->therapy_list.size(), 2);
  EXPECT_EQ(strategy->therapy_list[0], therapy1.get());
  EXPECT_EQ(strategy->therapy_list[1], therapy2.get());
}

TEST_F(AdaptiveCyclingStrategyTest, GetTherapy) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  strategy->add_therapy(therapy3.get());
  
  // Should start with first therapy (index 0)
  Therapy* selected_therapy = strategy->get_therapy(person.get());
  EXPECT_EQ(selected_therapy, therapy1.get());
  
  // Manually switch to next therapy (index 1)
  strategy->switch_therapy();
  selected_therapy = strategy->get_therapy(person.get());
  EXPECT_EQ(selected_therapy, therapy2.get());
  
  // Switch again (index 2)
  strategy->switch_therapy();
  selected_therapy = strategy->get_therapy(person.get());
  EXPECT_EQ(selected_therapy, therapy3.get());
  
  // Switch again (should cycle back to index 0)
  strategy->switch_therapy();
  selected_therapy = strategy->get_therapy(person.get());
  EXPECT_EQ(selected_therapy, therapy1.get());
}

TEST_F(AdaptiveCyclingStrategyTest, SwitchTherapy) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Initial index should be 0
  EXPECT_EQ(strategy->index, 0);
  
  // Switch therapy
  strategy->switch_therapy();
  
  // Index should now be 1
  EXPECT_EQ(strategy->index, 1);
  
  // Switch again, should wrap around to index 0
  strategy->switch_therapy();
  EXPECT_EQ(strategy->index, 0);
}

TEST_F(AdaptiveCyclingStrategyTest, UpdateEndOfTimeStepAtSwitchTime) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set current time to match latest_switch_time
  const int current_time = 100;
  Model::get_scheduler()->set_current_time(current_time);
  strategy->latest_switch_time = current_time;
  
  // Initial index
  EXPECT_EQ(strategy->index, 0);
  
  // Call update_end_of_time_step
  strategy->update_end_of_time_step();
  
  // Index should have increased
  EXPECT_EQ(strategy->index, 1);
}

TEST_F(AdaptiveCyclingStrategyTest, UpdateEndOfTimeStepWithTriggerValue) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set current time
  const int current_time = 200;
  Model::get_scheduler()->set_current_time(current_time);
  strategy->latest_switch_time = 0; // No recent switch
  
  // Set TF value above trigger threshold
  set_tf_value_for_therapy(therapy1->get_id(), 0.2); // Above the 0.15 trigger
  
  // Initial state
  EXPECT_EQ(strategy->index, 0);
  EXPECT_EQ(strategy->latest_switch_time, 0);
  
  // Call update_end_of_time_step
  strategy->update_end_of_time_step();
  
  // latest_switch_time should be updated to schedule future switch
  EXPECT_EQ(strategy->latest_switch_time, current_time + strategy->delay_until_actual_trigger);
  
  // Index should not have changed yet
  EXPECT_EQ(strategy->index, 0);
}

TEST_F(AdaptiveCyclingStrategyTest, UpdateEndOfTimeStepWithTriggerValueTooSoon) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set current time and recent switch time
  const int current_time = 200;
  Model::get_scheduler()->set_current_time(current_time);
  strategy->latest_switch_time = current_time - 50; // Switched 50 days ago (within turn_off_days)
  
  // Set TF value above trigger threshold
  set_tf_value_for_therapy(therapy1->get_id(), 0.2); // Above the 0.15 trigger
  
  // Initial state
  const int initial_switch_time = strategy->latest_switch_time;
  
  // Call update_end_of_time_step
  strategy->update_end_of_time_step();
  
  // latest_switch_time should not change because we're within turn_off_days
  EXPECT_EQ(strategy->latest_switch_time, initial_switch_time);
}

TEST_F(AdaptiveCyclingStrategyTest, AdjustStartedTimePoint) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Manually change index and latest_switch_time
  strategy->index = 1;
  strategy->latest_switch_time = 100;
  
  // Adjust start time point
  strategy->adjust_started_time_point(200);
  
  // Index should be reset to 0
  EXPECT_EQ(strategy->index, 0);
  
  // latest_switch_time should be reset to -1
  EXPECT_EQ(strategy->latest_switch_time, -1);
}

TEST_F(AdaptiveCyclingStrategyTest, ToString) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Test the string representation
  std::string expected = "1-TestAdaptiveCyclingStrategy-101,102";
  EXPECT_EQ(strategy->to_string(), expected);
}

TEST_F(AdaptiveCyclingStrategyTest, MonthlyUpdate) {
  // Monthly update does nothing in AdaptiveCyclingStrategy but should not crash
  EXPECT_NO_THROW(strategy->monthly_update());
}
