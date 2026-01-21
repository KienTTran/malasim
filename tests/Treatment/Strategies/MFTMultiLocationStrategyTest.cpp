#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "Treatment/Strategies/MFTMultiLocationStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Configuration/Config.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"
#include "Utils/TypeDef.h"

class MFTMultiLocationStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<MFTMultiLocationStrategy>();
    strategy->id = 1;
    strategy->name = "TestMFTMultiLocationStrategy";
    
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
    
    // Set up distribution parameters
    // Assuming 2 locations with 3 therapies
    
    // Start distributions
    DoubleVector2 start_dist;
    start_dist.push_back({0.7, 0.2, 0.1}); // Location 0
    start_dist.push_back({0.5, 0.3, 0.2}); // Location 1
    strategy->start_distribution = start_dist;
    
    // Peak distributions
    DoubleVector2 peak_dist;
    peak_dist.push_back({0.3, 0.5, 0.2}); // Location 0
    peak_dist.push_back({0.2, 0.5, 0.3}); // Location 1
    strategy->peak_distribution = peak_dist;
    
    // Current distributions (initially same as start)
    strategy->distribution = start_dist;
    
    // Timing parameters
    strategy->starting_time = 0;
    strategy->peak_after = 100;
    
    // Create test persons at different locations
    person_loc0 = std::make_unique<Person>();
    person_loc0->set_location(0);
    
    person_loc1 = std::make_unique<Person>();
    person_loc1->set_location(1);
  }

  void TearDown() override {
    person_loc1.reset();
    person_loc0.reset();
    therapies.clear();
    strategy.reset();
    test_fixtures::cleanup_test_files();
  }
  
  // Add all therapies to the strategy
  void add_all_therapies_to_strategy() {
    for (auto therapy : therapies) {
      strategy->add_therapy(therapy);
    }
  }
  
  // Helper to count therapy distribution
  std::map<int, int> count_therapy_distribution(Person* person, int iterations) {
    std::map<int, int> counts;
    for (int i = 0; i < iterations; i++) {
      Therapy* selected = strategy->get_therapy(person);
      counts[selected->get_id()]++;
    }
    return counts;
  }

  std::unique_ptr<MFTMultiLocationStrategy> strategy;
  std::vector<Therapy*> therapies;
  std::unique_ptr<Person> person_loc0;
  std::unique_ptr<Person> person_loc1;
};

TEST_F(MFTMultiLocationStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestMFTMultiLocationStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::MFTMultiLocation);
  EXPECT_TRUE(strategy->therapy_list.empty());
  EXPECT_EQ(strategy->starting_time, 0);
  EXPECT_EQ(strategy->peak_after, 100);
  
  // Check distributions
  ASSERT_EQ(strategy->distribution.size(), 2);
  ASSERT_EQ(strategy->distribution[0].size(), 3);
  ASSERT_EQ(strategy->distribution[1].size(), 3);
  
  // Location 0 distribution
  EXPECT_DOUBLE_EQ(strategy->distribution[0][0], 0.7);
  EXPECT_DOUBLE_EQ(strategy->distribution[0][1], 0.2);
  EXPECT_DOUBLE_EQ(strategy->distribution[0][2], 0.1);
  
  // Location 1 distribution
  EXPECT_DOUBLE_EQ(strategy->distribution[1][0], 0.5);
  EXPECT_DOUBLE_EQ(strategy->distribution[1][1], 0.3);
  EXPECT_DOUBLE_EQ(strategy->distribution[1][2], 0.2);
}

TEST_F(MFTMultiLocationStrategyTest, AddTherapy) {
  // Add first therapy
  strategy->add_therapy(therapies[0]);
  
  ASSERT_EQ(strategy->therapy_list.size(), 1);
  EXPECT_EQ(strategy->therapy_list[0], therapies[0]);
  
  // Add second therapy
  strategy->add_therapy(therapies[1]);
  
  ASSERT_EQ(strategy->therapy_list.size(), 2);
  EXPECT_EQ(strategy->therapy_list[0], therapies[0]);
  EXPECT_EQ(strategy->therapy_list[1], therapies[1]);
  
  // Add third therapy
  strategy->add_therapy(therapies[2]);
  
  ASSERT_EQ(strategy->therapy_list.size(), 3);
  EXPECT_EQ(strategy->therapy_list[0], therapies[0]);
  EXPECT_EQ(strategy->therapy_list[1], therapies[1]);
  EXPECT_EQ(strategy->therapy_list[2], therapies[2]);
}

TEST_F(MFTMultiLocationStrategyTest, ToString) {
  // Add all therapies
  add_all_therapies_to_strategy();
  
  // Test the string representation
  std::string expected = "1-TestMFTMultiLocationStrategy-" + std::to_string(therapies[0]->get_id()) + "::" + std::to_string(therapies[1]->get_id()) + "::" + std::to_string(therapies[2]->get_id());
  EXPECT_EQ(strategy->to_string(), expected);
}

TEST_F(MFTMultiLocationStrategyTest, GetTherapyByLocation) {
  // Add all therapies
  add_all_therapies_to_strategy();
  
  // Test that the therapy selection follows the distribution for location 0
  // This is a probabilistic test, so run many iterations
  const int iterations = 10000;
  auto counts_loc0 = count_therapy_distribution(person_loc0.get(), iterations);
  
  // Calculate proportions for location 0
  double prop1_loc0 = static_cast<double>(counts_loc0[therapies[0]->get_id()]) / iterations;
  double prop2_loc0 = static_cast<double>(counts_loc0[therapies[1]->get_id()]) / iterations;
  double prop3_loc0 = static_cast<double>(counts_loc0[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions for location 0 are approximately correct (allow 5% error)
  EXPECT_NEAR(prop1_loc0, 0.7, 0.05);
  EXPECT_NEAR(prop2_loc0, 0.2, 0.05);
  EXPECT_NEAR(prop3_loc0, 0.1, 0.05);
  
  // Test that the therapy selection follows the distribution for location 1
  auto counts_loc1 = count_therapy_distribution(person_loc1.get(), iterations);
  
  // Calculate proportions for location 1
  double prop1_loc1 = static_cast<double>(counts_loc1[therapies[0]->get_id()]) / iterations;
  double prop2_loc1 = static_cast<double>(counts_loc1[therapies[1]->get_id()]) / iterations;
  double prop3_loc1 = static_cast<double>(counts_loc1[therapies[2]->get_id()]) / iterations;
  
  // Check that proportions for location 1 are approximately correct (allow 5% error)
  EXPECT_NEAR(prop1_loc1, 0.5, 0.05);
  EXPECT_NEAR(prop2_loc1, 0.3, 0.05);
  EXPECT_NEAR(prop3_loc1, 0.2, 0.05);
}

TEST_F(MFTMultiLocationStrategyTest, AdjustStartedTimePoint) {
  // Add therapies
  add_all_therapies_to_strategy();
  
  // Set current time
  const int current_time = 50;
  
  // Adjust start time point
  strategy->adjust_started_time_point(current_time);
  
  // Verify that starting_time is updated
  EXPECT_EQ(strategy->starting_time, current_time);
}

TEST_F(MFTMultiLocationStrategyTest, MonthlyUpdateLinearDistributionChange) {
  // Add therapies
  add_all_therapies_to_strategy();
  
  // Set starting time and current time (halfway to peak)
  strategy->starting_time = 0;
  strategy->peak_after = 100;
  const int current_time = 50; // Halfway to peak
  Model::get_scheduler()->set_current_time(current_time);
  
  // Call monthly_update
  strategy->monthly_update();
  
  // Verify that distribution has been updated for each location halfway between start and peak
  // Location 0
  EXPECT_NEAR(strategy->distribution[0][0], 0.5, 0.001); // Halfway between 0.7 and 0.3
  EXPECT_NEAR(strategy->distribution[0][1], 0.35, 0.001); // Halfway between 0.2 and 0.5
  EXPECT_NEAR(strategy->distribution[0][2], 0.15, 0.001); // Halfway between 0.1 and 0.2
  
  // Location 1
  EXPECT_NEAR(strategy->distribution[1][0], 0.35, 0.001); // Halfway between 0.5 and 0.2
  EXPECT_NEAR(strategy->distribution[1][1], 0.4, 0.001); // Halfway between 0.3 and 0.5
  EXPECT_NEAR(strategy->distribution[1][2], 0.25, 0.001); // Halfway between 0.2 and 0.3
}

TEST_F(MFTMultiLocationStrategyTest, MonthlyUpdateAfterPeakReached) {
  // Add therapies
  add_all_therapies_to_strategy();
  
  // Set starting time and current time (after peak)
  strategy->starting_time = 0;
  strategy->peak_after = 100;
  const int current_time = 150; // After peak
  Model::get_scheduler()->set_current_time(current_time);
  
  // Set distribution to peak values first (as if peak was already reached)
  strategy->distribution = strategy->peak_distribution;
  
  // Call monthly_update
  strategy->monthly_update();
  
  // Verify that distribution remains at peak values
  // Location 0
  EXPECT_NEAR(strategy->distribution[0][0], 0.3, 0.001);
  EXPECT_NEAR(strategy->distribution[0][1], 0.5, 0.001);
  EXPECT_NEAR(strategy->distribution[0][2], 0.2, 0.001);
  
  // Location 1
  EXPECT_NEAR(strategy->distribution[1][0], 0.2, 0.001);
  EXPECT_NEAR(strategy->distribution[1][1], 0.5, 0.001);
  EXPECT_NEAR(strategy->distribution[1][2], 0.3, 0.001);
}

TEST_F(MFTMultiLocationStrategyTest, MonthlyUpdateWithInflationFactor) {
  // Add therapies
  add_all_therapies_to_strategy();
  
  // Configure for inflation mode
  strategy->peak_after = -1; // Special value to trigger inflation mode
  
  // Set current time
  const int current_time = 50;
  Model::get_scheduler()->set_current_time(current_time);
  
  // Reset distribution to known values
  strategy->distribution[0] = {0.5, 0.25, 0.25};
  strategy->distribution[1] = {0.6, 0.2, 0.2};
  
  // Call monthly_update
  strategy->monthly_update();
  
  // Verify that distribution has been updated according to inflation factor
  // This depends on the inflation factor from the config - assuming some inflation occurred
  EXPECT_GT(strategy->distribution[0][0], 0.5); // First therapy should have increased share
  EXPECT_LT(strategy->distribution[0][1], 0.25); // Other therapies should have decreased share
  EXPECT_LT(strategy->distribution[0][2], 0.25);
  
  EXPECT_GT(strategy->distribution[1][0], 0.6); // First therapy should have increased share
  EXPECT_LT(strategy->distribution[1][1], 0.2); // Other therapies should have decreased share
  EXPECT_LT(strategy->distribution[1][2], 0.2);
  
  // Check that all distributions still sum to 1.0
  double sum0 = strategy->distribution[0][0] + strategy->distribution[0][1] + strategy->distribution[0][2];
  double sum1 = strategy->distribution[1][0] + strategy->distribution[1][1] + strategy->distribution[1][2];
  EXPECT_NEAR(sum0, 1.0, 0.001);
  EXPECT_NEAR(sum1, 1.0, 0.001);
}
