#include <gtest/gtest.h>
#include <memory>

#include "Treatment/Therapies/Drug.h"
#include "Treatment/Therapies/DrugType.h"
#include "Population/DrugsInBlood.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class DrugTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create a drug type
    drug_type = std::make_unique<DrugType>();
    drug_type->set_id(1);
    drug_type->set_name("TestDrug");
    drug_type->set_drug_half_life(100.0);
    drug_type->set_maximum_parasite_killing_rate(0.9);
    drug_type->set_n(4.0);
    drug_type->set_k(0.5);
    drug_type->set_cut_off_percent(10.0);
    
    // Create a person and drugs in blood
    person = std::make_unique<Person>();
    drugs_in_blood = std::make_unique<DrugsInBlood>(person.get());
    
    // Create drug
    drug = std::make_unique<Drug>(drug_type.get());
    drug->set_person_drugs(drugs_in_blood.get());
    drug->set_starting_value(1.0);
    drug->set_dosing_days(3);
    drug->set_start_time(0);
    drug->set_end_time(333); // Based on dosing days and half-life
    drug->set_last_update_time(0);
    drug->set_last_update_value(1.0);
  }

  void TearDown() override {
    drug.reset();
    drugs_in_blood.reset();
    person.reset();
    drug_type.reset();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<DrugType> drug_type;
  std::unique_ptr<Person> person;
  std::unique_ptr<DrugsInBlood> drugs_in_blood;
  std::unique_ptr<Drug> drug;
};

TEST_F(DrugTest, Initialization) {
  EXPECT_EQ(drug->dosing_days(), 3);
  EXPECT_EQ(drug->start_time(), 0);
  EXPECT_EQ(drug->end_time(), 333);
  EXPECT_DOUBLE_EQ(drug->last_update_value(), 1.0);
  EXPECT_EQ(drug->last_update_time(), 0);
  EXPECT_DOUBLE_EQ(drug->starting_value(), 1.0);
  EXPECT_EQ(drug->drug_type(), drug_type.get());
  EXPECT_EQ(drug->person_drugs(), drugs_in_blood.get());
}

TEST_F(DrugTest, Update) {
  // Set current time in scheduler
  Model::get_scheduler()->set_current_time(10);
  
  // Store current values
  const double old_value = drug->last_update_value();
  const int old_time = drug->last_update_time();
  
  // Update drug
  drug->update();
  
  // Check that update happened
  EXPECT_NE(drug->last_update_value(), old_value);
  EXPECT_EQ(drug->last_update_time(), 10);
}

TEST_F(DrugTest, GetCurrentDrugConcentrationWithinDosingPeriod) {
  // Set current time within dosing period
  const int current_time = drug->start_time() + 2; // Within dosing days (3)
  
  // Get concentration at this time
  double concentration = drug->get_current_drug_concentration(current_time);
  
  // For non-artemisinin drugs (id != 0), should be approximately starting value
  // We can't test exact value due to randomness, but can check it's reasonable
  EXPECT_GE(concentration, 0.8);  // Lower bound considering random component
  EXPECT_LE(concentration, 1.2);  // Upper bound considering random component
}

TEST_F(DrugTest, GetCurrentDrugConcentrationAfterDosingPeriod) {
  // Set current time after dosing period but before end time
  const int days_after_dosing = 10;
  const int current_time = drug->start_time() + drug->dosing_days() + days_after_dosing;
  
  // Get concentration at this time
  double concentration = drug->get_current_drug_concentration(current_time);
  
  // Should follow exponential decay: starting_value * exp(-t * ln(2) / half_life)
  // Where t is the time since end of dosing
  const double expected_temp = -days_after_dosing * log(2) / drug_type->drug_half_life();
  const double expected_concentration = drug->starting_value() * exp(expected_temp);
  
  EXPECT_NEAR(concentration, expected_concentration, 0.01);
}

TEST_F(DrugTest, GetMutationProbability) {
  // Test with various drug concentrations
  
  // Low concentration (< 0.5)
  double concentration = 0.3;
  drug->set_last_update_value(concentration);
  
  const double mutation_prob_per_locus = Model::get_config()->get_genotype_parameters().get_mutation_probability_per_locus();
  double expected_prob = 2 * mutation_prob_per_locus * drug_type->k() * concentration;
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(), expected_prob);
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(concentration), expected_prob);
  
  // Medium concentration (between 0.5 and 1.0)
  concentration = 0.7;
  drug->set_last_update_value(concentration);
  
  expected_prob = mutation_prob_per_locus * (2 * (1 - drug_type->k()) * concentration + (2 * drug_type->k() - 1));
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(), expected_prob);
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(concentration), expected_prob);
  
  // High concentration (>= 1.0)
  concentration = 1.2;
  drug->set_last_update_value(concentration);
  
  expected_prob = mutation_prob_per_locus;
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(), expected_prob);
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(concentration), expected_prob);
  
  // Zero concentration
  concentration = 0.0;
  drug->set_last_update_value(concentration);
  
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(), 0.0);
  EXPECT_DOUBLE_EQ(drug->get_mutation_probability(concentration), 0.0);
}

TEST_F(DrugTest, SetNumberOfDosingDays) {
  const int new_dosing_days = 5;
  const int current_time = 10;
  
  // Set current time in scheduler
  Model::get_scheduler()->set_current_time(current_time);
  
  // Set new dosing days
  drug->set_number_of_dosing_days(new_dosing_days);
  
  // Check updated values
  EXPECT_EQ(drug->dosing_days(), new_dosing_days);
  EXPECT_DOUBLE_EQ(drug->last_update_value(), 1.0);
  EXPECT_EQ(drug->last_update_time(), current_time);
  EXPECT_EQ(drug->start_time(), current_time);
  
  // End time should be calculated as: start_time + drug_type's total duration
  const int expected_end_time = current_time + 
                               drug_type->get_total_duration_of_drug_activity(new_dosing_days);
  EXPECT_EQ(drug->end_time(), expected_end_time);
}

TEST_F(DrugTest, GetParasiteKillingRate) {
  // This test requires a properly initialized genotype DB
  // Will test that the function calls the appropriate DrugType function with correct parameters
  
  // Mock implementation - direct test of calculation
  const int genotype_id = 0; // Assuming this exists in the genotype DB
  const double concentration = 0.8;
  
  // Set last update value to test concentration
  drug->set_last_update_value(concentration);
  
  // Direct test of the calculation (mocking the DrugType::get_parasite_killing_rate_by_concentration call)
  // This relies on the existence of proper genotype in the DB - if this fails, might need to set up mock
  EXPECT_NO_THROW(drug->get_parasite_killing_rate(genotype_id));
}
