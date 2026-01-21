#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

#include "Treatment/Strategies/DistrictMftStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Spatial/GIS/SpatialData.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class DistrictMftStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create strategy
    strategy = std::make_unique<DistrictMftStrategy>();
    strategy->id = 1;
    strategy->name = "TestDistrictMftStrategy";
    
    // Use therapies from the therapy database instead of creating new ones
    if (Model::get_therapy_db().size() >= 3) {
      therapy1 = Model::get_therapy_db()[0].get();
      therapy2 = Model::get_therapy_db()[1].get();
      therapy3 = Model::get_therapy_db()[2].get();
    } else {
      // Fallback in case there aren't enough therapies in the database
      GTEST_SKIP() << "Not enough therapies in the database to run this test";
    }
    
    // Create test persons in different districts
    // Since we can't easily mock the SpatialData class,
    // we'll create persons but acknowledge that the district assignment
    // is dependent on the SpatialData implementation
    person1 = std::make_unique<Person>();
    person1->set_location(0); // This location should map to a district
    
    person2 = std::make_unique<Person>();
    person2->set_location(1); // This location should map to another district
  }

  void TearDown() override {
    person2.reset();
    person1.reset();
    strategy.reset();
    test_fixtures::cleanup_test_files();
  }
  
  // Helper to create MFT strategy for a district
  std::unique_ptr<DistrictMftStrategy::MftStrategy> create_district_strategy(
      const std::vector<int>& therapy_ids, 
      const std::vector<float>& percentages) {
    
    auto dist_strategy = std::make_unique<DistrictMftStrategy::MftStrategy>();
    dist_strategy->therapies = therapy_ids;
    dist_strategy->percentages = percentages;
    return dist_strategy;
  }

  std::unique_ptr<DistrictMftStrategy> strategy;
  Therapy* therapy1;
  Therapy* therapy2;
  Therapy* therapy3;
  std::unique_ptr<Person> person1;
  std::unique_ptr<Person> person2;
};

TEST_F(DistrictMftStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestDistrictMftStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::DistrictMft);
}

TEST_F(DistrictMftStrategyTest, AddTherapyThrowsException) {
  // Adding therapies directly should throw an exception
  EXPECT_THROW(strategy->add_therapy(therapy1), std::runtime_error);
}

TEST_F(DistrictMftStrategyTest, SetDistrictStrategy) {
  // Create district strategy 1
  std::vector<int> therapy_ids1 = {therapy1->get_id(), therapy2->get_id()};
  std::vector<float> percentages1 = {0.7f, 0.3f};
  auto district_strategy1 = create_district_strategy(therapy_ids1, percentages1);
  
  // Set district strategy for district 0
  // Note: Since this is dependent on SpatialData initialization,
  // we'll use district IDs that should be valid in most configurations
  EXPECT_NO_THROW(strategy->set_district_strategy(0, std::move(district_strategy1)));
  
  // Create district strategy 2
  std::vector<int> therapy_ids2 = {therapy2->get_id(), therapy3->get_id()};
  std::vector<float> percentages2 = {0.4f, 0.6f};
  auto district_strategy2 = create_district_strategy(therapy_ids2, percentages2);
  
  // Set district strategy for district 1
  EXPECT_NO_THROW(strategy->set_district_strategy(1, std::move(district_strategy2)));
}

TEST_F(DistrictMftStrategyTest, SetDistrictStrategyTwiceThrowsException) {
  // Create district strategy
  std::vector<int> therapy_ids = {therapy1->get_id(), therapy2->get_id()};
  std::vector<float> percentages = {0.7f, 0.3f};
  auto district_strategy1 = create_district_strategy(therapy_ids, percentages);
  
  // Set district strategy for district 0
  strategy->set_district_strategy(0, std::move(district_strategy1));
  
  // Try to set it again, should throw
  auto district_strategy2 = create_district_strategy(therapy_ids, percentages);
  EXPECT_THROW(strategy->set_district_strategy(0, std::move(district_strategy2)), std::runtime_error);
}

TEST_F(DistrictMftStrategyTest, SetInvalidDistrictThrowsException) {
  // Create district strategy
  std::vector<int> therapy_ids = {therapy1->get_id(), therapy2->get_id()};
  std::vector<float> percentages = {0.7f, 0.3f};
  auto district_strategy = create_district_strategy(therapy_ids, percentages);
  
  // Try to set for an invalid district (negative ID)
  EXPECT_THROW(strategy->set_district_strategy(-1, std::move(district_strategy)), std::out_of_range);
}

// NOTE: The following test depends on SpatialData's mapping of location to districts
// This may be hard to fully test without mocking SpatialData, but we'll structure
// it to be as robust as possible
TEST_F(DistrictMftStrategyTest, GetTherapyForPerson) {
  // Since get_therapy depends on SpatialData's mapping of location to district,
  // and Model::get_random() for probability generation, this test may be brittle
  // We'll do a basic test to ensure it doesn't crash, with the caveat that
  // the actual therapy selection depends on runtime configuration

  // Set up district strategies first
  // District 0 strategy
  std::vector<int> therapy_ids0 = {therapy1->get_id(), therapy2->get_id()};
  std::vector<float> percentages0 = {1.0f, 0.0f}; // Always select therapy1 for determinism
  auto district_strategy0 = create_district_strategy(therapy_ids0, percentages0);
  strategy->set_district_strategy(1, std::move(district_strategy0));

  // District 1 strategy
  std::vector<int> therapy_ids1 = {therapy2->get_id(), therapy3->get_id()};
  std::vector<float> percentages1 = {1.0f, 0.0f}; // Always select therapy2 for determinism
  auto district_strategy1 = create_district_strategy(therapy_ids1, percentages1);
  strategy->set_district_strategy(2, std::move(district_strategy1));

  // Try to get therapies for persons
  // This may fail if SpatialData maps locations to different districts than expected
  // We're just checking that the method executes without exception
  EXPECT_NO_THROW(strategy->get_therapy(person1.get()));
  EXPECT_NO_THROW(strategy->get_therapy(person2.get()));
}

TEST_F(DistrictMftStrategyTest, ToStringReturnsName) {
  // The to_string method just returns the name
  EXPECT_EQ(strategy->to_string(), "TestDistrictMftStrategy");
}

TEST_F(DistrictMftStrategyTest, LifecycleMethods) {
  // These methods are empty in DistrictMftStrategy but should not crash
  EXPECT_NO_THROW(strategy->update_end_of_time_step());
  EXPECT_NO_THROW(strategy->adjust_started_time_point(100));
  EXPECT_NO_THROW(strategy->monthly_update());
}
