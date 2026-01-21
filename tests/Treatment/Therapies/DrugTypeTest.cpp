#include <gtest/gtest.h>
#include <memory>

#include "Treatment/Therapies/DrugType.h"
#include "Configuration/Config.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class DrugTypeTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    drug_type = std::make_unique<DrugType>();
    drug_type->set_id(1);
    drug_type->set_name("TestDrug");
    drug_type->set_drug_half_life(100.0);
    drug_type->set_maximum_parasite_killing_rate(0.9);
    drug_type->set_n(4.0);
    drug_type->set_k(0.5);
    drug_type->set_cut_off_percent(10.0);
    drug_type->base_EC50 = 0.75;
    
    // Set age-specific values
    drug_type->age_group_specific_drug_concentration_sd() = {0.5, 0.4, 0.3};
    drug_type->age_specific_drug_absorption() = {0.8, 0.9, 1.0};
  }

  void TearDown() override {
    drug_type.reset();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<DrugType> drug_type;
};

TEST_F(DrugTypeTest, Initialization) {
  EXPECT_EQ(drug_type->id(), 1);
  EXPECT_EQ(drug_type->name(), "TestDrug");
  EXPECT_DOUBLE_EQ(drug_type->drug_half_life(), 100.0);
  EXPECT_DOUBLE_EQ(drug_type->maximum_parasite_killing_rate(), 0.9);
  EXPECT_DOUBLE_EQ(drug_type->k(), 0.5);
  EXPECT_DOUBLE_EQ(drug_type->cut_off_percent(), 10.0);
  EXPECT_DOUBLE_EQ(drug_type->n(), 4.0);
  EXPECT_DOUBLE_EQ(drug_type->base_EC50, 0.75);
  
  // Check age-specific values
  ASSERT_EQ(drug_type->age_group_specific_drug_concentration_sd().size(), 3);
  EXPECT_DOUBLE_EQ(drug_type->age_group_specific_drug_concentration_sd()[0], 0.5);
  EXPECT_DOUBLE_EQ(drug_type->age_group_specific_drug_concentration_sd()[1], 0.4);
  EXPECT_DOUBLE_EQ(drug_type->age_group_specific_drug_concentration_sd()[2], 0.3);
  
  ASSERT_EQ(drug_type->age_specific_drug_absorption().size(), 3);
  EXPECT_DOUBLE_EQ(drug_type->age_specific_drug_absorption()[0], 0.8);
  EXPECT_DOUBLE_EQ(drug_type->age_specific_drug_absorption()[1], 0.9);
  EXPECT_DOUBLE_EQ(drug_type->age_specific_drug_absorption()[2], 1.0);
}

TEST_F(DrugTypeTest, GetParasiteKillingRateByConcentration) {
  // Test with different concentration values and EC50 values
  double concentration = 1.0;
  double EC50_power_n = std::pow(0.5, drug_type->n());
  
  double expected_killing_rate = drug_type->maximum_parasite_killing_rate() * 
                                 (std::pow(concentration, drug_type->n()) / 
                                 (std::pow(concentration, drug_type->n()) + EC50_power_n));
                                 
  EXPECT_NEAR(drug_type->get_parasite_killing_rate_by_concentration(concentration, EC50_power_n), 
              expected_killing_rate, 1e-10);
  
  // Test with different concentration
  concentration = 2.0;
  expected_killing_rate = drug_type->maximum_parasite_killing_rate() * 
                         (std::pow(concentration, drug_type->n()) / 
                         (std::pow(concentration, drug_type->n()) + EC50_power_n));
                         
  EXPECT_NEAR(drug_type->get_parasite_killing_rate_by_concentration(concentration, EC50_power_n), 
              expected_killing_rate, 1e-10);
  
  // Test with concentration = 0
  EXPECT_DOUBLE_EQ(drug_type->get_parasite_killing_rate_by_concentration(0, EC50_power_n), 0);
}

TEST_F(DrugTypeTest, SetAndGetN) {
  // Test setting and getting n value
  drug_type->set_n(2.5);
  EXPECT_DOUBLE_EQ(drug_type->n(), 2.5);
  
  drug_type->set_n(3.0);
  EXPECT_DOUBLE_EQ(drug_type->n(), 3.0);
}

TEST_F(DrugTypeTest, GetTotalDurationOfDrugActivity) {
  // Test with different dosing days
  // Expected: dosing_days + ceil(drug_half_life_ * LOG2_10)
  // LOG2_10 ≈ 3.32192809489
  
  int dosing_days = 3;
  int expected_duration = dosing_days + std::ceil(drug_type->drug_half_life() * 3.32192809489);
  EXPECT_EQ(drug_type->get_total_duration_of_drug_activity(dosing_days), expected_duration);
  
  dosing_days = 5;
  expected_duration = dosing_days + std::ceil(drug_type->drug_half_life() * 3.32192809489);
  EXPECT_EQ(drug_type->get_total_duration_of_drug_activity(dosing_days), expected_duration);
}

TEST_F(DrugTypeTest, PopulateResistantAALocations) {
  // This test requires proper initialization of the Model with genotype parameters
  // Test basic functionality
  drug_type->resistant_aa_locations.clear();
  EXPECT_TRUE(drug_type->resistant_aa_locations.empty());
  
  // Call populate_resistant_aa_locations
  drug_type->populate_resistant_aa_locations();
  
  // The test result depends on the initialization in the sample input file
  // We're just checking that the function doesn't crash and that resistant locations
  // might be populated depending on the configuration
}
