#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "Treatment/Strategies/CyclingStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class CyclingStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<CyclingStrategy>();
    strategy->id = 1;
    strategy->name = "TestCyclingStrategy";
    strategy->cycling_time = 30; // 30 days per cycle
    
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
  }

  void TearDown() override {
    person.reset();
    therapy3.reset();
    therapy2.reset();
    therapy1.reset();
    strategy.reset();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<CyclingStrategy> strategy;
  std::unique_ptr<Therapy> therapy1;
  std::unique_ptr<Therapy> therapy2;
  std::unique_ptr<Therapy> therapy3;
  std::unique_ptr<Person> person;
};

TEST_F(CyclingStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestCyclingStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::Cycling);
  EXPECT_TRUE(strategy->therapy_list.empty());
  EXPECT_EQ(strategy->index, 0);
  EXPECT_EQ(strategy->cycling_time, 30);
  EXPECT_EQ(strategy->next_switching_day, 0);
}

TEST_F(CyclingStrategyTest, AddTherapy) {
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

TEST_F(CyclingStrategyTest, GetTherapy) {
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

TEST_F(CyclingStrategyTest, SwitchTherapy) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Initial index should be 0
  EXPECT_EQ(strategy->index, 0);
  
  // Set current time in scheduler
  const int current_time = 10;
  Model::get_scheduler()->set_current_time(current_time);
  
  // Switch therapy
  strategy->switch_therapy();
  
  // Index should now be 1
  EXPECT_EQ(strategy->index, 1);
  
  // Next switching day should be current time + cycling time
  EXPECT_EQ(strategy->next_switching_day, current_time + strategy->cycling_time);
  
  // Switch again, should wrap around to index 0
  strategy->switch_therapy();
  EXPECT_EQ(strategy->index, 0);
}

TEST_F(CyclingStrategyTest, UpdateEndOfTimeStep) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set current time and next switching day
  const int current_time = 100;
  Model::get_scheduler()->set_current_time(current_time);
  strategy->next_switching_day = current_time; // Set to trigger switch
  
  // Initial index
  EXPECT_EQ(strategy->index, 0);
  
  // Call update_end_of_time_step
  strategy->update_end_of_time_step();
  
  // Index should have increased
  EXPECT_EQ(strategy->index, 1);
  
  // Next switching day should be updated
  EXPECT_EQ(strategy->next_switching_day, current_time + strategy->cycling_time);
  
  // If current time is not switching day, no change should occur
  Model::get_scheduler()->set_current_time(current_time + 1); // Not a switching day
  strategy->update_end_of_time_step();
  
  // Index should remain the same
  EXPECT_EQ(strategy->index, 1);
}

TEST_F(CyclingStrategyTest, AdjustStartedTimePoint) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Manually change index
  strategy->index = 1;
  
  // Set current time
  const int current_time = 50;
  Model::get_scheduler()->set_current_time(current_time);
  
  // Adjust start time point
  strategy->adjust_started_time_point(current_time);
  
  // Index should be reset to 0
  EXPECT_EQ(strategy->index, 0);
  
  // Next switching day should be updated
  EXPECT_EQ(strategy->next_switching_day, current_time + strategy->cycling_time);
}

TEST_F(CyclingStrategyTest, ToString) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Test the string representation
  std::string expected = "1-TestCyclingStrategy-101,102";
  EXPECT_EQ(strategy->to_string(), expected);
}

TEST_F(CyclingStrategyTest, MonthlyUpdate) {
  // Monthly update does nothing in CyclingStrategy but should not crash
  EXPECT_NO_THROW(strategy->monthly_update());
}
