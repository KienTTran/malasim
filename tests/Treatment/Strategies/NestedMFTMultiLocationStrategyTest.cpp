#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "Treatment/Strategies/NestedMFTMultiLocationStrategy.h"
#include "Treatment/Strategies/SFTStrategy.h"
#include "Treatment/Strategies/MFTStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "Utils/TypeDef.h"

class NestedMFTMultiLocationStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Create nested strategy
    nested_strategy = std::make_unique<NestedMFTMultiLocationStrategy>();
    nested_strategy->id = 1;
    nested_strategy->name = "TestNestedMFTMultiLocationStrategy";
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
    
    // Create test persons at different locations
    person_loc0 = std::make_unique<Person>();
    person_loc0->set_location(0);
    
    person_loc1 = std::make_unique<Person>();
    person_loc1->set_location(1);
    
    // Set up distributions for the nested strategy
    // Assuming 2 locations and 2 child strategies
    DoubleVector2 start_dist;
    DoubleVector2 peak_dist;
    DoubleVector2 current_dist;
    
    // Location 0
    start_dist.push_back({0.7, 0.3});    // Location 0 start: 70% SFT, 30% MFT
    peak_dist.push_back({0.3, 0.7});     // Location 0 peak: 30% SFT, 70% MFT
    current_dist.push_back({0.7, 0.3});  // Location 0 initial: same as start
    
    // Location 1
    start_dist.push_back({0.6, 0.4});    // Location 1 start: 60% SFT, 40% MFT
    peak_dist.push_back({0.2, 0.8});     // Location 1 peak: 20% SFT, 80% MFT
    current_dist.push_back({0.6, 0.4});  // Location 1 initial: same as start
    
    nested_strategy->start_distribution = start_dist;
    nested_strategy->peak_distribution = peak_dist;
    nested_strategy->distribution = current_dist;
  }

  void TearDown() override {
    person_loc1.reset();
    person_loc0.reset();
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
  
  // Helper to count therapy distribution for a specific location
  std::map<int, int> count_therapy_distribution(Person* person, int iterations) {
    std::map<int, int> counts;
    for (int i = 0; i < iterations; i++) {
      Therapy* selected = nested_strategy->get_therapy(person);
      counts[selected->get_id()]++;
    }
    return counts;
  }

  std::unique_ptr<NestedMFTMultiLocationStrategy> nested_strategy;
  std::unique_ptr<SFTStrategy> sft_strategy;
  std::unique_ptr<MFTStrategy> mft_strategy;
  std::vector<Therapy*> therapies;
  std::unique_ptr<Person> person_loc0;
  std::unique_ptr<Person> person_loc1;
};

TEST_F(NestedMFTMultiLocationStrategyTest, Initialization) {
  EXPECT_EQ(nested_strategy->id, 1);
  EXPECT_EQ(nested_strategy->name, "TestNestedMFTMultiLocationStrategy");
  EXPECT_EQ(nested_strategy->get_type(), IStrategy::StrategyType::NestedMFTMultiLocation);
  EXPECT_TRUE(nested_strategy->strategy_list.empty());
  EXPECT_EQ(nested_strategy->starting_time, 0);
  EXPECT_EQ(nested_strategy->peak_after, 100);
  
  // Check distributions
  ASSERT_EQ(nested_strategy->distribution.size(), 2);
  ASSERT_EQ(nested_strategy->distribution[0].size(), 2);
  ASSERT_EQ(nested_strategy->distribution[1].size(), 2);
  
  // Location 0
  EXPECT_DOUBLE_EQ(nested_strategy->distribution[0][0], 0.7);
  EXPECT_DOUBLE_EQ(nested_strategy->distribution[0][1], 0.3);
  
  // Location 1
  EXPECT_DOUBLE_EQ(nested_strategy->distribution[1][0], 0.6);
  EXPECT_DOUBLE_EQ(nested_strategy->distribution[1][1], 0.4);
}

TEST_F(NestedMFTMultiLocationStrategyTest, AddStrategy) {
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

TEST_F(NestedMFTMultiLocationStrategyTest, AddTherapy) {
  // The add_therapy method is intentionally empty in NestedMFTMultiLocationStrategy
  // Test that calling it has no effect
  nested_strategy->add_therapy(therapies[0]);
  
  // Should not throw or change state
  EXPECT_TRUE(nested_strategy->strategy_list.empty());
}

TEST_F(NestedMFTMultiLocationStrategyTest, ToString) {
  // Add child strategies
  add_child_strategies();
  
  // Test the string representation
  std::string result = nested_strategy->to_string();
  EXPECT_TRUE(result.find("1-TestNestedMFTMultiLocationStrategy") != std::string::npos);
}

TEST_F(NestedMFTMultiLocationStrategyTest, GetTherapyByLocationInitialDistribution) {
  // Add child strategies
  add_child_strategies();
  
  // Test therapy selection for location 0
  const int iterations = 10000;
  auto counts_loc0 = count_therapy_distribution(person_loc0.get(), iterations);
  
  // Expected for Location 0: 
  // - 70% from SFT strategy (therapy1) = 70%
  // - 30% from MFT strategy (40% therapy2, 60% therapy3) = 12% therapy2, 18% therapy3
  
  double prop1_loc0 = static_cast<double>(counts_loc0[therapies[0]->get_id()]) / iterations;
  double prop2_loc0 = static_cast<double>(counts_loc0[therapies[1]->get_id()]) / iterations;
  double prop3_loc0 = static_cast<double>(counts_loc0[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions are approximately correct for location 0 (allow 5% error)
  EXPECT_NEAR(prop1_loc0, 0.7, 0.05);
  EXPECT_NEAR(prop2_loc0, 0.12, 0.05);
  EXPECT_NEAR(prop3_loc0, 0.18, 0.05);
  
  // Test therapy selection for location 1
  auto counts_loc1 = count_therapy_distribution(person_loc1.get(), iterations);
  
  // Expected for Location 1: 
  // - 60% from SFT strategy (therapy1) = 60%
  // - 40% from MFT strategy (40% therapy2, 60% therapy3) = 16% therapy2, 24% therapy3
  
  double prop1_loc1 = static_cast<double>(counts_loc1[therapies[0]->get_id()]) / iterations;
  double prop2_loc1 = static_cast<double>(counts_loc1[therapies[1]->get_id()]) / iterations;
  double prop3_loc1 = static_cast<double>(counts_loc1[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions are approximately correct for location 1 (allow 5% error)
  EXPECT_NEAR(prop1_loc1, 0.6, 0.05);
  EXPECT_NEAR(prop2_loc1, 0.16, 0.05);
  EXPECT_NEAR(prop3_loc1, 0.24, 0.05);
}

TEST_F(NestedMFTMultiLocationStrategyTest, UpdateEndOfTimeStep) {
  // Add child strategies
  add_child_strategies();
  
  // Should call update_end_of_time_step on each child strategy
  EXPECT_NO_THROW(nested_strategy->update_end_of_time_step());
  // Since we can't easily verify the calls were made, we just ensure it doesn't throw
}

TEST_F(NestedMFTMultiLocationStrategyTest, AdjustDistributionMidway) {
  // Add child strategies
  add_child_strategies();
  
  // Set starting time and current time (halfway to peak)
  nested_strategy->starting_time = 0;
  nested_strategy->peak_after = 100;
  const int current_time = 50; // Halfway to peak
  
  // Adjust distribution
  nested_strategy->adjust_distribution(current_time);
  
  // The actual calculation in adjust_distribution doesn't average, but directly applies
  // this formula: (peak - start) * (time - starting_time) / peak_after + start
  // Let's update our expectations to match the actual calculation:
  
  // Location 0
  // SFT: start 0.7, peak 0.3, at midway should be 0.7 + (0.3-0.7)*(50/100) = 0.7 - 0.2 = 0.5
  EXPECT_NEAR(nested_strategy->distribution[0][0], 0.5, 0.001); 
  // MFT: start 0.3, peak 0.7, at midway should be 0.3 + (0.7-0.3)*(50/100) = 0.3 + 0.2 = 0.5
  EXPECT_NEAR(nested_strategy->distribution[0][1], 0.5, 0.001); 
  
  // Location 1
  // SFT: start 0.6, peak 0.2, at midway should be 0.6 + (0.2-0.6)*(50/100) = 0.6 - 0.2 = 0.4
  EXPECT_NEAR(nested_strategy->distribution[1][0], 0.4, 0.001); 
  // MFT: start 0.4, peak 0.8, at midway should be 0.4 + (0.8-0.4)*(50/100) = 0.4 + 0.2 = 0.6
  EXPECT_NEAR(nested_strategy->distribution[1][1], 0.6, 0.001); 
}

TEST_F(NestedMFTMultiLocationStrategyTest, AdjustDistributionAfterPeak) {
  // Add child strategies
  add_child_strategies();
  
  // Set starting time and current time (after peak)
  nested_strategy->starting_time = 0;
  nested_strategy->peak_after = 100;
  const int current_time = 150; // After peak
  
  // Adjust distribution
  nested_strategy->adjust_distribution(current_time);
  
  // Verify that distribution is set to peak values
  // Location 0
  EXPECT_NEAR(nested_strategy->distribution[0][0], 0.3, 0.001); // Peak value
  EXPECT_NEAR(nested_strategy->distribution[0][1], 0.7, 0.001); // Peak value
  
  // Location 1
  EXPECT_NEAR(nested_strategy->distribution[1][0], 0.2, 0.001); // Peak value
  EXPECT_NEAR(nested_strategy->distribution[1][1], 0.8, 0.001); // Peak value
}

TEST_F(NestedMFTMultiLocationStrategyTest, AdjustDistributionWithInflation) {
  // Add child strategies
  add_child_strategies();
  
  // Set peak_after to -1 to trigger inflation mode
  nested_strategy->peak_after = -1;
  
  // Reset distribution to known values
  nested_strategy->distribution[0] = {0.5, 0.5};
  nested_strategy->distribution[1] = {0.6, 0.4};
  
  // Adjust distribution using inflation mode
  nested_strategy->adjust_distribution(100); // Time doesn't matter for inflation mode
  
  // Verify that distribution has been updated according to inflation factor
  // This depends on the inflation factor from the config - assuming some inflation occurred
  EXPECT_GT(nested_strategy->distribution[0][0], 0.5); // First strategy should have increased share in location 0
  EXPECT_LT(nested_strategy->distribution[0][1], 0.5); // Second strategy should have decreased share in location 0
  
  EXPECT_GT(nested_strategy->distribution[1][0], 0.6); // First strategy should have increased share in location 1
  EXPECT_LT(nested_strategy->distribution[1][1], 0.4); // Second strategy should have decreased share in location 1
  
  // Check that all distributions still sum to 1.0
  double sum0 = nested_strategy->distribution[0][0] + nested_strategy->distribution[0][1];
  double sum1 = nested_strategy->distribution[1][0] + nested_strategy->distribution[1][1];
  EXPECT_NEAR(sum0, 1.0, 0.001);
  EXPECT_NEAR(sum1, 1.0, 0.001);
}

TEST_F(NestedMFTMultiLocationStrategyTest, MonthlyUpdate) {
  // Add child strategies
  add_child_strategies();
  
  // Set up the scheduler with a current time
  const int current_time = 50;
  Model::get_scheduler()->set_current_time(current_time);
  
  // Call monthly_update
  nested_strategy->monthly_update();
  
  // Verify that distribution has been updated
  // Location 0
  EXPECT_NEAR(nested_strategy->distribution[0][0], 0.5, 0.001); // Halfway between 0.7 and 0.3
  EXPECT_NEAR(nested_strategy->distribution[0][1], 0.5, 0.001); // Halfway between 0.3 and 0.7
  
  // Location 1
  EXPECT_NEAR(nested_strategy->distribution[1][0], 0.4, 0.001); // Halfway between 0.6 and 0.2
  EXPECT_NEAR(nested_strategy->distribution[1][1], 0.6, 0.001); // Halfway between 0.4 and 0.8
}

TEST_F(NestedMFTMultiLocationStrategyTest, GetTherapyWithUpdatedDistribution) {
  // Add child strategies
  add_child_strategies();
  
  // Update distribution to be halfway between start and peak
  nested_strategy->distribution[0] = {0.5, 0.5}; // Location 0
  nested_strategy->distribution[1] = {0.4, 0.6}; // Location 1
  
  // Test therapy selection for location 0 with updated distribution
  const int iterations = 10000;
  auto counts_loc0 = count_therapy_distribution(person_loc0.get(), iterations);
  
  // Expected for Location 0: 
  // - 50% from SFT strategy (therapy1) = 50%
  // - 50% from MFT strategy (40% therapy2, 60% therapy3) = 20% therapy2, 30% therapy3
  
  double prop1_loc0 = static_cast<double>(counts_loc0[therapies[0]->get_id()]) / iterations;
  double prop2_loc0 = static_cast<double>(counts_loc0[therapies[1]->get_id()]) / iterations;
  double prop3_loc0 = static_cast<double>(counts_loc0[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions are approximately correct for location 0 (allow 5% error)
  EXPECT_NEAR(prop1_loc0, 0.5, 0.05);
  EXPECT_NEAR(prop2_loc0, 0.2, 0.05);
  EXPECT_NEAR(prop3_loc0, 0.3, 0.05);
  
  // Test therapy selection for location 1 with updated distribution
  auto counts_loc1 = count_therapy_distribution(person_loc1.get(), iterations);
  
  // Expected for Location 1: 
  // - 40% from SFT strategy (therapy1) = 40%
  // - 60% from MFT strategy (40% therapy2, 60% therapy3) = 24% therapy2, 36% therapy3
  
  double prop1_loc1 = static_cast<double>(counts_loc1[therapies[0]->get_id()]) / iterations;
  double prop2_loc1 = static_cast<double>(counts_loc1[therapies[1]->get_id()]) / iterations;
  double prop3_loc1 = static_cast<double>(counts_loc1[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions are approximately correct for location 1 (allow 5% error)
  EXPECT_NEAR(prop1_loc1, 0.4, 0.05);
  EXPECT_NEAR(prop2_loc1, 0.24, 0.05);
  EXPECT_NEAR(prop3_loc1, 0.36, 0.05);
}
