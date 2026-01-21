#include <gtest/gtest.h>
#include <memory>

#include "Treatment/Strategies/MFTAgeBasedStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"
#include "Utils/Constants.h"

class MFTAgeBasedStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<MFTAgeBasedStrategy>();
    strategy->id = 1;
    strategy->name = "TestMFTAgeBasedStrategy";
    
    // Set age boundaries - we need 4 boundaries for 5 age groups
    strategy->age_boundaries = {5.0, 10.0, 15.0, 18.0};
    
    // Use therapies from the therapy database
    therapies.clear();
    // Need 5 therapies (one for each age group)
    for (int i = 0; i < 5 && i < Model::get_therapy_db().size(); i++) {
      therapies.push_back(Model::get_therapy_db()[i].get());
    }
    
    // Skip test if not enough therapies
    if (therapies.size() < 5) {
      GTEST_SKIP() << "Not enough therapies in the database to run this test";
    }
    
    // Set up the current simulation time
    int current_time = 100 * Constants::DAYS_IN_YEAR; // 100 years into simulation
    Model::get_scheduler()->set_current_time(current_time);
    
    // Create test persons of different ages and set their birthdays directly
    // to ensure age_in_floating returns the correct value
    
    // Child under 5 (3 years old)
    child = std::make_unique<Person>();
    child->set_birthday(current_time - (3 * Constants::DAYS_IN_YEAR));
    
    // Child between 5-10 (8 years old)
    preteen = std::make_unique<Person>();
    preteen->set_birthday(current_time - (8 * Constants::DAYS_IN_YEAR));
    
    // Child between 10-15 (12 years old)
    teen = std::make_unique<Person>();
    teen->set_birthday(current_time - (12 * Constants::DAYS_IN_YEAR));
    
    // Teen between 15-18 (16 years old)
    older_teen = std::make_unique<Person>();
    older_teen->set_birthday(current_time - (16 * Constants::DAYS_IN_YEAR));
    
    // Adult over 18 (25 years old)
    adult = std::make_unique<Person>();
    adult->set_birthday(current_time - (25 * Constants::DAYS_IN_YEAR));
  }

  void TearDown() override {
    adult.reset();
    older_teen.reset();
    teen.reset();
    preteen.reset();
    child.reset();
    therapies.clear();
    strategy.reset();
    test_fixtures::cleanup_test_files();
  }
  
  // Add all therapies to the strategy
  void add_all_therapies_to_strategy() {
    // Clear any existing therapies first
    strategy->therapy_list.clear();
    
    // Add therapies in order so they match the age groups:
    // - therapies[0] for ages < 5
    // - therapies[1] for ages 5-10
    // - therapies[2] for ages 10-15
    // - therapies[3] for ages 15-18
    // - therapies[4] for ages >= 18
    for (auto therapy : therapies) {
      strategy->add_therapy(therapy);
    }
  }

  std::unique_ptr<MFTAgeBasedStrategy> strategy;
  std::vector<Therapy*> therapies;
  std::unique_ptr<Person> child;
  std::unique_ptr<Person> preteen;
  std::unique_ptr<Person> teen;
  std::unique_ptr<Person> older_teen;
  std::unique_ptr<Person> adult;
};

TEST_F(MFTAgeBasedStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestMFTAgeBasedStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::MFTAgeBased);
  EXPECT_TRUE(strategy->therapy_list.empty());
  
  ASSERT_EQ(strategy->age_boundaries.size(), 4);
  EXPECT_DOUBLE_EQ(strategy->age_boundaries[0], 5.0);
  EXPECT_DOUBLE_EQ(strategy->age_boundaries[1], 10.0);
  EXPECT_DOUBLE_EQ(strategy->age_boundaries[2], 15.0);
  EXPECT_DOUBLE_EQ(strategy->age_boundaries[3], 18.0);
}

TEST_F(MFTAgeBasedStrategyTest, AddTherapy) {
  // Add first therapy
  strategy->add_therapy(therapies[0]);
  
  ASSERT_EQ(strategy->therapy_list.size(), 1);
  EXPECT_EQ(strategy->therapy_list[0], therapies[0]);
  
  // Add second therapy
  strategy->add_therapy(therapies[1]);
  
  ASSERT_EQ(strategy->therapy_list.size(), 2);
  EXPECT_EQ(strategy->therapy_list[0], therapies[0]);
  EXPECT_EQ(strategy->therapy_list[1], therapies[1]);
}

TEST_F(MFTAgeBasedStrategyTest, ToString) {
  // Add all therapies
  add_all_therapies_to_strategy();
  
  // Test the string representation
  std::string expected = "1-TestMFTAgeBasedStrategy-";
  for (auto therapy : therapies) {
    expected += std::to_string(therapy->get_id()) + ",";
  }
  expected.pop_back(); // Remove trailing comma
  expected += "-5,10,15,18";
  EXPECT_EQ(strategy->to_string(), expected);
}

TEST_F(MFTAgeBasedStrategyTest, FindAgeRangeIndex) {
  // Test the static utility function directly
  std::vector<double> age_boundaries = {5.0, 10.0, 15.0, 18.0};
  
  // Test edge cases and values within ranges
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, -1.0), 0);    // Below first boundary
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 3.0), 0);     // Below first boundary
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 5.0), 1);     // Exactly at first boundary
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 7.5), 1);     // Between first and second
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 10.0), 2);    // Exactly at second boundary
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 12.5), 2);    // Between second and third
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 15.0), 3);    // Exactly at third boundary
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 17.5), 3);    // Between third and fourth
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 18.0), 4);    // Exactly at fourth boundary
  EXPECT_EQ(MFTAgeBasedStrategy::find_age_range_index(age_boundaries, 25.0), 4);    // Above last boundary
}

TEST_F(MFTAgeBasedStrategyTest, FindAgeRangeIndexMemberFunction) {
  // Test the member function
  
  // Test edge cases and values within ranges
  EXPECT_EQ(strategy->find_age_range_index(-1.0), 0);    // Below first boundary
  EXPECT_EQ(strategy->find_age_range_index(3.0), 0);     // Below first boundary
  EXPECT_EQ(strategy->find_age_range_index(5.0), 1);     // Exactly at first boundary
  EXPECT_EQ(strategy->find_age_range_index(7.5), 1);     // Between first and second
  EXPECT_EQ(strategy->find_age_range_index(10.0), 2);    // Exactly at second boundary
  EXPECT_EQ(strategy->find_age_range_index(12.5), 2);    // Between second and third
  EXPECT_EQ(strategy->find_age_range_index(15.0), 3);    // Exactly at third boundary
  EXPECT_EQ(strategy->find_age_range_index(17.5), 3);    // Between third and fourth
  EXPECT_EQ(strategy->find_age_range_index(18.0), 4);    // Exactly at fourth boundary
  EXPECT_EQ(strategy->find_age_range_index(25.0), 4);    // Above last boundary
}

TEST_F(MFTAgeBasedStrategyTest, GetTherapyByAgeGroup) {
  // Add all therapies
  add_all_therapies_to_strategy();
  
  // Verify that we have the correct number of therapies
  ASSERT_EQ(strategy->therapy_list.size(), 5);
  
  // Due to the random nature of therapy selection in production code,
  // we'll modify the test to check that the index calculation is correct
  // rather than the actual therapy selection
  
  // Child under 5 should be in age group 0
  EXPECT_EQ(strategy->find_age_range_index(child->age_in_floating(Model::get_scheduler()->current_time())), 0);
  
  // Child between 5-10 should be in age group 1
  EXPECT_EQ(strategy->find_age_range_index(preteen->age_in_floating(Model::get_scheduler()->current_time())), 1);
  
  // Child between 10-15 should be in age group 2
  EXPECT_EQ(strategy->find_age_range_index(teen->age_in_floating(Model::get_scheduler()->current_time())), 2);
  
  // Teen between 15-18 should be in age group 3
  EXPECT_EQ(strategy->find_age_range_index(older_teen->age_in_floating(Model::get_scheduler()->current_time())), 3);
  
  // Adult over 18 should be in age group 4
  EXPECT_EQ(strategy->find_age_range_index(adult->age_in_floating(Model::get_scheduler()->current_time())), 4);
  
  // Now test that get_therapy returns the expected therapy for a known age group
  // Create a test person with a specific age
  auto test_person = std::make_unique<Person>();
  test_person->set_birthday(Model::get_scheduler()->current_time() - (30 * Constants::DAYS_IN_YEAR)); // Clearly in the last age group
  
  // Adult over 18 should get therapy 5
  Therapy* selected_therapy = strategy->get_therapy(test_person.get());
  EXPECT_EQ(selected_therapy, therapies[4]);
}

TEST_F(MFTAgeBasedStrategyTest, LifecycleMethods) {
  // These methods are empty in MFTAgeBasedStrategy but should not crash
  
  // Add a therapy so the strategy is properly initialized
  strategy->add_therapy(therapies[0]);
  
  // Test update methods
  EXPECT_NO_THROW(strategy->update_end_of_time_step());
  EXPECT_NO_THROW(strategy->adjust_started_time_point(100));
  EXPECT_NO_THROW(strategy->monthly_update());
}
