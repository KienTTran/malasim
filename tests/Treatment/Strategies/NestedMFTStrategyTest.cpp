#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "Treatment/Strategies/NestedMFTStrategy.h"
#include "Treatment/Strategies/SFTStrategy.h"
#include "Treatment/Strategies/MFTStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"

class NestedMFTStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Create nested strategy
    nested_strategy = std::make_unique<NestedMFTStrategy>();
    nested_strategy->id = 1;
    nested_strategy->name = "TestNestedMFTStrategy";
    nested_strategy->starting_time = 0;
    nested_strategy->peak_after = 100;
    
    // Create child strategies
    sft_strategy = std::make_unique<SFTStrategy>();
    sft_strategy->id = 10;
    sft_strategy->name = "TestSFTStrategy";
    
    mft_strategy = std::make_unique<MFTStrategy>();
    mft_strategy->id = 20;
    mft_strategy->name = "TestMFTStrategy";
    
    // Use therapies from the therapy database
    therapies.clear();
    // Need at least 3 therapies
    for (int i = 0; i < 3 && i < Model::get_therapy_db().size(); i++) {
      therapies.push_back(Model::get_therapy_db()[i].get());
    }
    
    // Skip test if not enough therapies
    if (therapies.size() < 3) {
      GTEST_SKIP() << "Not enough therapies in the database to run this test";
    }
    
    // Add therapies to child strategies
    sft_strategy->add_therapy(therapies[0]);
    
    mft_strategy->add_therapy(therapies[1]);
    mft_strategy->add_therapy(therapies[2]);
    mft_strategy->distribution = {0.4, 0.6};
    
    // Create test person
    person = std::make_unique<Person>();
    
    // Set up distributions for the nested strategy
    nested_strategy->start_distribution = {0.7, 0.3};    // Start: 70% SFT, 30% MFT
    nested_strategy->peak_distribution = {0.3, 0.7};     // Peak: 30% SFT, 70% MFT
    nested_strategy->distribution = {0.7, 0.3};         // Initial: same as start
  }

  void TearDown() override {
    person.reset();
    therapies.clear();
    mft_strategy.reset();
    sft_strategy.reset();
    nested_strategy.reset();
  }
  
  // Add child strategies to the nested strategy
  void add_child_strategies() {
    nested_strategy->add_strategy(sft_strategy.get());
    nested_strategy->add_strategy(mft_strategy.get());
  }
  
  // Helper to count therapy distribution
  std::map<int, int> count_therapy_distribution(int iterations) {
    std::map<int, int> counts;
    for (int i = 0; i < iterations; i++) {
      Therapy* selected = nested_strategy->get_therapy(person.get());
      counts[selected->get_id()]++;
    }
    return counts;
  }

  std::unique_ptr<NestedMFTStrategy> nested_strategy;
  std::unique_ptr<SFTStrategy> sft_strategy;
  std::unique_ptr<MFTStrategy> mft_strategy;
  std::vector<Therapy*> therapies;
  std::unique_ptr<Person> person;
};

TEST_F(NestedMFTStrategyTest, Initialization) {
  EXPECT_EQ(nested_strategy->id, 1);
  EXPECT_EQ(nested_strategy->name, "TestNestedMFTStrategy");
  EXPECT_EQ(nested_strategy->get_type(), IStrategy::StrategyType::NestedMFT);
  EXPECT_TRUE(nested_strategy->strategy_list.empty());
  EXPECT_EQ(nested_strategy->starting_time, 0);
  EXPECT_EQ(nested_strategy->peak_after, 100);
  
  // Check distributions
  ASSERT_EQ(nested_strategy->start_distribution.size(), 2);
  ASSERT_EQ(nested_strategy->peak_distribution.size(), 2);
  ASSERT_EQ(nested_strategy->distribution.size(), 2);
  
  EXPECT_DOUBLE_EQ(nested_strategy->start_distribution[0], 0.7);
  EXPECT_DOUBLE_EQ(nested_strategy->start_distribution[1], 0.3);
  
  EXPECT_DOUBLE_EQ(nested_strategy->peak_distribution[0], 0.3);
  EXPECT_DOUBLE_EQ(nested_strategy->peak_distribution[1], 0.7);
  
  EXPECT_DOUBLE_EQ(nested_strategy->distribution[0], 0.7);
  EXPECT_DOUBLE_EQ(nested_strategy->distribution[1], 0.3);
}

TEST_F(NestedMFTStrategyTest, AddStrategy) {
  // Add first strategy
  nested_strategy->add_strategy(sft_strategy.get());
  
  ASSERT_EQ(nested_strategy->strategy_list.size(), 1);
  EXPECT_EQ(nested_strategy->strategy_list[0], sft_strategy.get());
  
  // Add second strategy
  nested_strategy->add_strategy(mft_strategy.get());
  
  ASSERT_EQ(nested_strategy->strategy_list.size(), 2);
  EXPECT_EQ(nested_strategy->strategy_list[0], sft_strategy.get());
  EXPECT_EQ(nested_strategy->strategy_list[1], mft_strategy.get());
}

TEST_F(NestedMFTStrategyTest, AddTherapy) {
  // The add_therapy method is intentionally empty in NestedMFTStrategy
  // Test that calling it has no effect
  nested_strategy->add_therapy(therapies[0]);
  
  // Should not throw or change state
  EXPECT_TRUE(nested_strategy->strategy_list.empty());
}

TEST_F(NestedMFTStrategyTest, ToString) {
  // Add child strategies
  add_child_strategies();
  
  // Test the string representation
  std::string expected = "1-TestNestedMFTStrategy-0.7::0.3::";
  EXPECT_EQ(nested_strategy->to_string(), expected);
}

TEST_F(NestedMFTStrategyTest, GetTherapyInitialDistribution) {
  // Add child strategies
  add_child_strategies();
  
  // Test that the therapy selection follows the initial distribution
  // This is a probabilistic test, so run many iterations
  const int iterations = 10000;
  auto counts = count_therapy_distribution(iterations);
  
  // Expected: 
  // - 70% from SFT strategy (therapy1) = 70%
  // - 30% from MFT strategy (40% therapy2, 60% therapy3) = 12% therapy2, 18% therapy3
  
  double prop1 = static_cast<double>(counts[therapies[0]->get_id()]) / iterations;
  double prop2 = static_cast<double>(counts[therapies[1]->get_id()]) / iterations;
  double prop3 = static_cast<double>(counts[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions are approximately correct (allow 5% error)
  EXPECT_NEAR(prop1, 0.7, 0.05);
  EXPECT_NEAR(prop2, 0.12, 0.05);
  EXPECT_NEAR(prop3, 0.18, 0.05);
}


TEST_F(NestedMFTStrategyTest, UpdateEndOfTimeStep) {
  // Add child strategies
  add_child_strategies();
  
  // Should call update_end_of_time_step on each child strategy
  EXPECT_NO_THROW(nested_strategy->update_end_of_time_step());
  // Since we can't easily verify the calls were made, we just ensure it doesn't throw
}

TEST_F(NestedMFTStrategyTest, AdjustDistributionMidway) {
  // Add child strategies
  add_child_strategies();
  
  // Set starting time and current time (halfway to peak)
  nested_strategy->starting_time = 0;
  nested_strategy->peak_after = 100;
  const int current_time = 50; // Halfway to peak
  
  // Adjust distribution
  nested_strategy->adjust_distribution(current_time);
  
  // Verify that distribution has been updated halfway between start and peak
  EXPECT_NEAR(nested_strategy->distribution[0], 0.5, 0.001); // Halfway between 0.7 and 0.3
  EXPECT_NEAR(nested_strategy->distribution[1], 0.5, 0.001); // Halfway between 0.3 and 0.7
}

TEST_F(NestedMFTStrategyTest, AdjustDistributionAfterPeak) {
  // Add child strategies
  add_child_strategies();
  
  // Set starting time and current time (after peak)
  nested_strategy->starting_time = 0;
  nested_strategy->peak_after = 100;
  const int current_time = 150; // After peak
  
  // Adjust distribution
  nested_strategy->adjust_distribution(current_time);
  
  // Verify that distribution is set to peak values
  EXPECT_NEAR(nested_strategy->distribution[0], 0.3, 0.001); // Peak value
  EXPECT_NEAR(nested_strategy->distribution[1], 0.7, 0.001); // Peak value
}

TEST_F(NestedMFTStrategyTest, MonthlyUpdate) {
  // Add child strategies
  add_child_strategies();
  
  // Set up the scheduler with a current time
  const int current_time = 50;
  Model::get_scheduler()->set_current_time(current_time);
  
  // Call monthly_update
  nested_strategy->monthly_update();
  
  // Verify that distribution has been updated
  EXPECT_NEAR(nested_strategy->distribution[0], 0.5, 0.001); // Halfway between 0.7 and 0.3
  EXPECT_NEAR(nested_strategy->distribution[1], 0.5, 0.001); // Halfway between 0.3 and 0.7
}

TEST_F(NestedMFTStrategyTest, GetTherapyWithUpdatedDistribution) {
  // Add child strategies
  add_child_strategies();
  
  // Update distribution to be 50-50
  nested_strategy->distribution = {0.5, 0.5};
  
  // Test that the therapy selection follows the updated distribution
  const int iterations = 10000;
  auto counts = count_therapy_distribution(iterations);
  
  // Expected: 
  // - 50% from SFT strategy (therapy1) = 50%
  // - 50% from MFT strategy (40% therapy2, 60% therapy3) = 20% therapy2, 30% therapy3
  
  double prop1 = static_cast<double>(counts[therapies[0]->get_id()]) / iterations;
  double prop2 = static_cast<double>(counts[therapies[1]->get_id()]) / iterations;
  double prop3 = static_cast<double>(counts[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions are approximately correct (allow 5% error)
  EXPECT_NEAR(prop1, 0.5, 0.05);
  EXPECT_NEAR(prop2, 0.2, 0.05);
  EXPECT_NEAR(prop3, 0.3, 0.05);
}

TEST_F(NestedMFTStrategyTest, GetTherapyWithFinalDistribution) {
  // Add child strategies
  add_child_strategies();
  
  // Update distribution to final state
  nested_strategy->distribution = nested_strategy->peak_distribution;
  
  // Test that the therapy selection follows the peak distribution
  const int iterations = 10000;
  auto counts = count_therapy_distribution(iterations);
  
  // Expected: 
  // - 30% from SFT strategy (therapy1) = 30%
  // - 70% from MFT strategy (40% therapy2, 60% therapy3) = 28% therapy2, 42% therapy3
  
  double prop1 = static_cast<double>(counts[therapies[0]->get_id()]) / iterations;
  double prop2 = static_cast<double>(counts[therapies[1]->get_id()]) / iterations;
  double prop3 = static_cast<double>(counts[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions are approximately correct (allow 5% error)
  EXPECT_NEAR(prop1, 0.3, 0.05);
  EXPECT_NEAR(prop2, 0.28, 0.05);
  EXPECT_NEAR(prop3, 0.42, 0.05);
}
