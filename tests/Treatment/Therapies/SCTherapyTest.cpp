#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "Treatment/Therapies/SCTherapy.h"
#include "Treatment/Therapies/DrugType.h"
#include "Treatment/Therapies/DrugDatabase.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"

class SCTherapyTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Create a single-compound therapy
    sc_therapy = std::make_unique<SCTherapy>();
    sc_therapy->set_id(1);
    sc_therapy->set_name("SC Therapy Test");
    sc_therapy->set_testing_day(7);
    sc_therapy->set_full_compliance(true);
    
    // Set up drug database with sample drugs
    setup_drug_database();
  }

  void TearDown() override {
    sc_therapy.reset();
    
    // Model cleanup will handle drug_db cleanup
  }
  
  void setup_drug_database() {
    // Create drug types
    auto drug_db = std::make_unique<DrugDatabase>();
    
    // Create artemisinin drug with ID 0
    auto artemisinin = std::make_unique<DrugType>();
    artemisinin->set_id(0);
    artemisinin->set_name("Artemisinin");
    artemisinin->set_drug_half_life(10.0);
    drug_db->add(std::move(artemisinin));
    
    // Create other drugs
    for (int i = 1; i < 3; i++) {
      auto dt = std::make_unique<DrugType>();
      dt->set_id(i);
      dt->set_name("Drug" + std::to_string(i));
      dt->set_drug_half_life(100.0 + i * 10);
      drug_db->add(std::move(dt));
    }
    
    // Set the drug database in the model
    Model::get_instance()->set_drug_db(std::move(drug_db));
  }

  std::unique_ptr<SCTherapy> sc_therapy;
};

TEST_F(SCTherapyTest, Initialization) {
  EXPECT_EQ(sc_therapy->get_id(), 1);
  EXPECT_EQ(sc_therapy->get_name(), "SC Therapy Test");
  EXPECT_EQ(sc_therapy->get_testing_day(), 7);
  EXPECT_TRUE(sc_therapy->drug_ids.empty());
  EXPECT_TRUE(sc_therapy->full_compliance());
  EXPECT_EQ(sc_therapy->artemisinin_id, -1);
  EXPECT_TRUE(sc_therapy->dosing_day.empty());
  EXPECT_TRUE(sc_therapy->pr_completed_days.empty());
}

TEST_F(SCTherapyTest, AddDrugNonArtemisinin) {
  sc_therapy->add_drug(1); // Add Drug1 (non-artemisinin)
  
  ASSERT_EQ(sc_therapy->drug_ids.size(), 1);
  EXPECT_EQ(sc_therapy->drug_ids[0], 1);
  EXPECT_EQ(sc_therapy->artemisinin_id, -1); // Not artemisinin
}

TEST_F(SCTherapyTest, AddDrugArtemisinin) {
  sc_therapy->add_drug(0); // Add Drug0 (artemisinin)
  
  ASSERT_EQ(sc_therapy->drug_ids.size(), 1);
  EXPECT_EQ(sc_therapy->drug_ids[0], 0);
  EXPECT_EQ(sc_therapy->artemisinin_id, 0); // Artemisinin ID set
}

TEST_F(SCTherapyTest, AddMultipleDrugs) {
  sc_therapy->add_drug(0); // Add Drug0 (artemisinin)
  sc_therapy->add_drug(1); // Add Drug1 (non-artemisinin)
  
  ASSERT_EQ(sc_therapy->drug_ids.size(), 2);
  EXPECT_EQ(sc_therapy->drug_ids[0], 0);
  EXPECT_EQ(sc_therapy->drug_ids[1], 1);
  EXPECT_EQ(sc_therapy->artemisinin_id, 0); // Artemisinin ID set
}

TEST_F(SCTherapyTest, CalculateMaxDosingDay) {
  // Set dosing days
  sc_therapy->dosing_day = {1, 3, 2};
  
  // Calculate max dosing day
  sc_therapy->calculate_max_dosing_day();
  
  // Check result
  EXPECT_EQ(sc_therapy->get_max_dosing_day(), 3);
}

TEST_F(SCTherapyTest, CalculateMaxDosingDaySingle) {
  // Set a single dosing day
  sc_therapy->dosing_day = {5};
  
  // Calculate max dosing day
  sc_therapy->calculate_max_dosing_day();
  
  // Check result
  EXPECT_EQ(sc_therapy->get_max_dosing_day(), 5);
}

TEST_F(SCTherapyTest, GetMaxDosingDayBeforeCalculation) {
  // For death tests, we need to match the assertion message pattern
  // The pattern varies by platform/compiler, but this should catch both GCC and MSVC formats
  EXPECT_THROW(sc_therapy->get_max_dosing_day(), std::runtime_error);
  
  // Set dosing days
  sc_therapy->dosing_day = {3};
  
  // Calculate max dosing day
  sc_therapy->calculate_max_dosing_day();
  
  // Now get_max_dosing_day() should work
  EXPECT_EQ(3, sc_therapy->get_max_dosing_day());
}

TEST_F(SCTherapyTest, Print) {
  // Add some drugs
  sc_therapy->add_drug(0); // Artemisinin
  sc_therapy->add_drug(1); // Drug1
  
  // Set dosing days
  sc_therapy->dosing_day = {2, 3, 1};
  
  // Test print output
  std::stringstream ss;
  sc_therapy->print(ss);
  
  // Expected format: DrugName1+DrugName2(dosing_day1,dosing_day2,dosing_day3)
  EXPECT_EQ(ss.str(), "Artemisinin+Drug1(2,3,1)");
}
