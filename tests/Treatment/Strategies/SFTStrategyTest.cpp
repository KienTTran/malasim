#include <gtest/gtest.h>
#include <memory>

#include "Treatment/Strategies/SFTStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class SFTStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<SFTStrategy>();
    strategy->id = 1;
    strategy->name = "TestSFTStrategy";
    
    // Create therapies
    therapy1 = std::make_unique<Therapy>();
    therapy1->set_id(101);
    therapy1->set_name("Therapy 1");
    
    therapy2 = std::make_unique<Therapy>();
    therapy2->set_id(102);
    therapy2->set_name("Therapy 2");
    
    // Create a test person
    person = std::make_unique<Person>();
  }

  void TearDown() override {
    person.reset();
    therapy2.reset();
    therapy1.reset();
    strategy.reset();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<SFTStrategy> strategy;
  std::unique_ptr<Therapy> therapy1;
  std::unique_ptr<Therapy> therapy2;
  std::unique_ptr<Person> person;
};

TEST_F(SFTStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestSFTStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::SFT);
  EXPECT_TRUE(strategy->get_therapy_list().empty());
}

TEST_F(SFTStrategyTest, AddTherapy) {
  // Add first therapy
  strategy->add_therapy(therapy1.get());
  
  ASSERT_EQ(strategy->get_therapy_list().size(), 1);
  EXPECT_EQ(strategy->get_therapy_list()[0], therapy1.get());
  
  // Add second therapy
  strategy->add_therapy(therapy2.get());
  
  ASSERT_EQ(strategy->get_therapy_list().size(), 2);
  EXPECT_EQ(strategy->get_therapy_list()[0], therapy1.get());
  EXPECT_EQ(strategy->get_therapy_list()[1], therapy2.get());
}

TEST_F(SFTStrategyTest, GetTherapy) {
  // Add therapies
  strategy->add_therapy(therapy1.get());
  strategy->add_therapy(therapy2.get());
  
  // SFT strategy should always return the first therapy
  Therapy* selected_therapy = strategy->get_therapy(person.get());
  EXPECT_EQ(selected_therapy, therapy1.get());
}

TEST_F(SFTStrategyTest, ToString) {
  // Add therapy
  strategy->add_therapy(therapy1.get());
  
  // Test the string representation
  std::string expected = "1-TestSFTStrategy-101";
  EXPECT_EQ(strategy->to_string(), expected);
}

TEST_F(SFTStrategyTest, LifecycleMethods) {
  // These methods do nothing in SFTStrategy but should not crash
  
  // Add a therapy so the strategy is properly initialized
  strategy->add_therapy(therapy1.get());
  
  // Test update methods
  EXPECT_NO_THROW(strategy->update_end_of_time_step());
  EXPECT_NO_THROW(strategy->adjust_started_time_point(100));
  EXPECT_NO_THROW(strategy->monthly_update());
}

TEST_F(SFTStrategyTest, IsStrategy) {
  EXPECT_TRUE(strategy->is_strategy("TestSFTStrategy"));
  EXPECT_FALSE(strategy->is_strategy("OtherStrategy"));
}

TEST_F(SFTStrategyTest, SetAndGetTherapyList) {
  // Create therapy list
  std::vector<Therapy*> therapies = {therapy1.get(), therapy2.get()};
  
  // Set therapy list
  strategy->set_therapy_list(therapies);
  
  // Check if therapy list is set correctly
  auto result_list = strategy->get_therapy_list();
  ASSERT_EQ(result_list.size(), 2);
  EXPECT_EQ(result_list[0], therapy1.get());
  EXPECT_EQ(result_list[1], therapy2.get());
}
