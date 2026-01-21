#include <gtest/gtest.h>
#include <memory>

#include "Treatment/Therapies/DrugDatabase.h"
#include "Treatment/Therapies/DrugType.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class DrugDatabaseTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    drug_db = std::make_unique<DrugDatabase>();
  }

  void TearDown() override {
    drug_db.reset();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<DrugDatabase> drug_db;
  
  // Helper method to create a drug type with the given ID
  std::unique_ptr<DrugType> create_drug_type(int id, const std::string& name) {
    auto drug_type = std::make_unique<DrugType>();
    drug_type->set_id(id);
    drug_type->set_name(name);
    drug_type->set_drug_half_life(100.0 + id); // Different half-life for each drug
    return drug_type;
  }
};

TEST_F(DrugDatabaseTest, InitialState) {
  EXPECT_EQ(drug_db->size(), 0);
}

TEST_F(DrugDatabaseTest, AddDrug) {
  auto drug_type = create_drug_type(0, "Drug0");
  DrugType* drug_type_ptr = drug_type.get();
  
  // Add the drug to the database
  drug_db->add(std::move(drug_type));
  
  // Check database size
  EXPECT_EQ(drug_db->size(), 1);
  
  // Check the drug was added correctly
  ASSERT_NE(drug_db->at(0), nullptr);
  EXPECT_EQ(drug_db->at(0)->id(), 0);
  EXPECT_EQ(drug_db->at(0)->name(), "Drug0");
  EXPECT_DOUBLE_EQ(drug_db->at(0)->drug_half_life(), 100.0);
  
  // The original drug_type should be moved to the database
  EXPECT_EQ(drug_db->at(0).get(), drug_type_ptr);
}

TEST_F(DrugDatabaseTest, AddMultipleDrugs) {
  // Add multiple drugs
  drug_db->add(create_drug_type(0, "Drug0"));
  drug_db->add(create_drug_type(1, "Drug1"));
  drug_db->add(create_drug_type(2, "Drug2"));
  
  // Check database size
  EXPECT_EQ(drug_db->size(), 3);
  
  // Check each drug
  ASSERT_NE(drug_db->at(0), nullptr);
  EXPECT_EQ(drug_db->at(0)->id(), 0);
  EXPECT_EQ(drug_db->at(0)->name(), "Drug0");
  
  ASSERT_NE(drug_db->at(1), nullptr);
  EXPECT_EQ(drug_db->at(1)->id(), 1);
  EXPECT_EQ(drug_db->at(1)->name(), "Drug1");
  
  ASSERT_NE(drug_db->at(2), nullptr);
  EXPECT_EQ(drug_db->at(2)->id(), 2);
  EXPECT_EQ(drug_db->at(2)->name(), "Drug2");
}

TEST_F(DrugDatabaseTest, AddDrugWithGap) {
  // Add drugs with non-consecutive IDs
  drug_db->add(create_drug_type(0, "Drug0"));
  drug_db->add(create_drug_type(3, "Drug3")); // Skip IDs 1 and 2
  
  // Check database size - should be 4 to accommodate ID 3
  EXPECT_EQ(drug_db->size(), 4);
  
  // Check drugs at positions 0 and 3
  ASSERT_NE(drug_db->at(0), nullptr);
  EXPECT_EQ(drug_db->at(0)->id(), 0);
  EXPECT_EQ(drug_db->at(0)->name(), "Drug0");
  
  ASSERT_NE(drug_db->at(3), nullptr);
  EXPECT_EQ(drug_db->at(3)->id(), 3);
  EXPECT_EQ(drug_db->at(3)->name(), "Drug3");
  
  // Positions 1 and 2 should be nullptr
  EXPECT_EQ(drug_db->at(1), nullptr);
  EXPECT_EQ(drug_db->at(2), nullptr);
}

TEST_F(DrugDatabaseTest, AddDrugWithSameId) {
  // Add a drug
  drug_db->add(create_drug_type(1, "DrugA"));
  
  // Add another drug with the same ID
  drug_db->add(create_drug_type(1, "DrugB"));
  
  // Check that the drug was replaced
  ASSERT_NE(drug_db->at(1), nullptr);
  EXPECT_EQ(drug_db->at(1)->name(), "DrugB");
}

TEST_F(DrugDatabaseTest, Clear) {
  // Add some drugs
  drug_db->add(create_drug_type(0, "Drug0"));
  drug_db->add(create_drug_type(1, "Drug1"));
  
  EXPECT_EQ(drug_db->size(), 2);
  
  // Clear the database
  drug_db->clear();
  
  // Database should be empty
  EXPECT_EQ(drug_db->size(), 0);
}
