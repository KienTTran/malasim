#include <gtest/gtest.h>
#include <memory>
#include <map>

#include "Treatment/Strategies/MFTStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "Utils/Random.h"

class MFTStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<MFTStrategy>();
    strategy->id = 1;
    strategy->name = "TestMFTStrategy";
    
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
  }

  std::unique_ptr<MFTStrategy> strategy;
  std::unique_ptr<Therapy> therapy1;
  std::unique_ptr<Therapy> therapy2;
  std::unique_ptr<Therapy> therapy3;
  std::unique_ptr<Person> person;
  
  // Helper to count distribution of therapies
  std::map<int, int> count_therapy_distribution(int iterations) {
    std::map<int, int> counts;
    for (int i = 0; i < iterations; i++) {
      Therapy* selected = strategy->get_therapy(person.get());
      counts[selected->get_id()]++;
    }
    return counts;
  }
};

TEST_F(MFTStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestMFTStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::MFT);
  EXPECT_TRUE(strategy->therapy_list.empty());
  EXPECT_TRUE(strategy->distribution.empty());
}

TEST_F(MFTStrategyTest, AddTherapy) {
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

TEST_F(MFTStrategyTest, GetTherapyDistribution) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  strategy->add_therapy(therapy3.get());
  
  // Set distribution
  strategy->distribution = {0.2, 0.3, 0.5};
  
  // With fixed random seed, we could test exact distribution
  // For now, we'll just test that it returns a valid therapy
  Therapy* selected_therapy = strategy->get_therapy(person.get());
  EXPECT_TRUE(selected_therapy == therapy1.get() || 
              selected_therapy == therapy2.get() || 
              selected_therapy == therapy3.get());
  
  // Test that the distribution approximately matches what we expect
  // Note: This is a probabilistic test and may occasionally fail due to random variation
  // We run many iterations to make this less likely
  const int iterations = 10000;
  auto counts = count_therapy_distribution(iterations);
  
  // Calculate proportions
  double prop1 = static_cast<double>(counts[therapy1->get_id()]) / iterations;
  double prop2 = static_cast<double>(counts[therapy2->get_id()]) / iterations;
  double prop3 = static_cast<double>(counts[therapy3->get_id()]) / iterations;
  
  // Check that proportions are approximately correct (allow 5% error)
  EXPECT_NEAR(prop1, 0.2, 0.05);
  EXPECT_NEAR(prop2, 0.3, 0.05);
  EXPECT_NEAR(prop3, 0.5, 0.05);
}

TEST_F(MFTStrategyTest, GetTherapyWithEmptyDistribution) {
  // Add therapies but don't set distribution
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // With no distribution set, should default to the last therapy
  Therapy* selected_therapy = strategy->get_therapy(person.get());
  EXPECT_EQ(selected_therapy, therapy2.get());
}

TEST_F(MFTStrategyTest, ToString) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // Set distribution
  strategy->distribution = {0.4, 0.6};
  
  // Test the string representation
  std::string expected = "1-TestMFTStrategy-101,102-0.4,0.6";
  EXPECT_EQ(strategy->to_string(), expected);
}

TEST_F(MFTStrategyTest, LifecycleMethods) {
  // These methods do nothing in MFTStrategy but should not crash
  
  // Add a therapy so the strategy is properly initialized
  strategy->add_therapy(therapy1.get());
  
  // Test update methods
  EXPECT_NO_THROW(strategy->update_end_of_time_step());
  EXPECT_NO_THROW(strategy->adjust_started_time_point(100));
  EXPECT_NO_THROW(strategy->monthly_update());
}

TEST_F(MFTStrategyTest, IsStrategy) {
  EXPECT_TRUE(strategy->is_strategy("TestMFTStrategy"));
  EXPECT_FALSE(strategy->is_strategy("OtherStrategy"));
}
