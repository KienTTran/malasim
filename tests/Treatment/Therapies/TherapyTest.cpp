#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "Treatment/Therapies/Therapy.h"
#include "Treatment/Therapies/DrugType.h"
#include "Treatment/Therapies/DrugDatabase.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class TherapyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create a therapy
    therapy = std::make_unique<Therapy>();
    therapy->set_id(1);
    therapy->set_name("Test Therapy");
    therapy->set_testing_day(7);
    
    // Set up drug database with sample drugs
    setup_drug_database();
  }

  void TearDown() override {
    therapy.reset();
    
    // Model cleanup will handle drug_db cleanup
    test_fixtures::cleanup_test_files();
  }
  
  void setup_drug_database() {
    // Create drug types
    auto drug_db = std::make_unique<DrugDatabase>();
    
    for (int i = 0; i < 3; i++) {
      auto dt = std::make_unique<DrugType>();
      dt->set_id(i);
      dt->set_name("Drug" + std::to_string(i));
      dt->set_drug_half_life(100.0 + i * 10);
      drug_db->add(std::move(dt));
    }
    
    // Set the drug database in the model
    Model::get_instance()->set_drug_db(std::move(drug_db));
  }

  std::unique_ptr<Therapy> therapy;
};

TEST_F(TherapyTest, Initialization) {
  EXPECT_EQ(therapy->get_id(), 1);
  EXPECT_EQ(therapy->get_name(), "Test Therapy");
  EXPECT_EQ(therapy->get_testing_day(), 7);
  EXPECT_TRUE(therapy->drug_ids.empty());
}

TEST_F(TherapyTest, AddDrug) {
  therapy->add_drug(0); // Add Drug0
  
  ASSERT_EQ(therapy->drug_ids.size(), 1);
  EXPECT_EQ(therapy->drug_ids[0], 0);
  
  therapy->add_drug(1); // Add Drug1
  
  ASSERT_EQ(therapy->drug_ids.size(), 2);
  EXPECT_EQ(therapy->drug_ids[0], 0);
  EXPECT_EQ(therapy->drug_ids[1], 1);
}

TEST_F(TherapyTest, StreamOperator) {
  // Add some drugs
  therapy->add_drug(0); // Drug0
  therapy->add_drug(2); // Drug2
  
  // Test custom name
  std::stringstream ss;
  ss << *therapy;
  EXPECT_EQ(ss.str(), "Test Therapy");
  
  // Test default name generation
  therapy->set_name("");
  ss.str("");
  ss << *therapy;
  EXPECT_EQ(ss.str(), "Drug0+Drug2");
  
  // Test with single drug
  therapy = std::make_unique<Therapy>();
  therapy->add_drug(1);
  therapy->set_name("");
  ss.str("");
  ss << *therapy;
  EXPECT_EQ(ss.str(), "Drug1");
}
