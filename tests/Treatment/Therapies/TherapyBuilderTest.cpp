#include <gtest/gtest.h>
#include <memory>
#include <yaml-cpp/yaml.h>

#include "Treatment/Therapies/TherapyBuilder.h"
#include "Treatment/Therapies/Therapy.h"
#include "Treatment/Therapies/SCTherapy.h"
#include "Treatment/Therapies/MACTherapy.h"
#include "Treatment/Therapies/DrugType.h"
#include "Treatment/Therapies/DrugDatabase.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"

class TherapyBuilderTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Set up drug database with sample drugs
    setup_drug_database();
  }

  void TearDown() override {
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
};

TEST_F(TherapyBuilderTest, BuildSimpleTherapy) {
  // Create a simple YAML node representing a single-compound therapy
  YAML::Node node;
  node["drug_ids"].push_back(0);  // Artemisinin
  node["drug_ids"].push_back(1);  // Drug1
  node["dosing_days"].push_back(1);
  node["dosing_days"].push_back(2);
  node["dosing_days"].push_back(3);
  
  // Build the therapy
  std::unique_ptr<Therapy> therapy = TherapyBuilder::build(node, 5);
  
  // Check that we got a SCTherapy
  SCTherapy* sc_therapy = dynamic_cast<SCTherapy*>(therapy.get());
  ASSERT_NE(sc_therapy, nullptr);
  
  // Check properties
  EXPECT_EQ(sc_therapy->get_id(), 5);
  ASSERT_EQ(sc_therapy->drug_ids.size(), 2);
  EXPECT_EQ(sc_therapy->drug_ids[0], 0);
  EXPECT_EQ(sc_therapy->drug_ids[1], 1);
  ASSERT_EQ(sc_therapy->dosing_day.size(), 3);
  EXPECT_EQ(sc_therapy->dosing_day[0], 1);
  EXPECT_EQ(sc_therapy->dosing_day[1], 2);
  EXPECT_EQ(sc_therapy->dosing_day[2], 3);
  EXPECT_EQ(sc_therapy->artemisinin_id, 0);
  EXPECT_TRUE(sc_therapy->full_compliance());
}

TEST_F(TherapyBuilderTest, BuildSimpleTherapyWithComplianceRates) {
  // Create a simple YAML node with pr_completed_days
  YAML::Node node;
  node["drug_ids"].push_back(1);  // Drug1
  node["dosing_days"].push_back(3);
  node["pr_completed_days"].push_back(0.3);
  node["pr_completed_days"].push_back(0.4);
  node["pr_completed_days"].push_back(0.3);
  
  // Build the therapy
  std::unique_ptr<Therapy> therapy = TherapyBuilder::build(node, 6);
  
  // Check that we got a SCTherapy
  SCTherapy* sc_therapy = dynamic_cast<SCTherapy*>(therapy.get());
  ASSERT_NE(sc_therapy, nullptr);
  
  // Check properties
  EXPECT_EQ(sc_therapy->get_id(), 6);
  ASSERT_EQ(sc_therapy->drug_ids.size(), 1);
  EXPECT_EQ(sc_therapy->drug_ids[0], 1);
  ASSERT_EQ(sc_therapy->dosing_day.size(), 1);
  EXPECT_EQ(sc_therapy->dosing_day[0], 3);
  EXPECT_FALSE(sc_therapy->full_compliance());
  
  // Check pr_completed_days values
  ASSERT_EQ(sc_therapy->pr_completed_days.size(), 3);
  EXPECT_DOUBLE_EQ(sc_therapy->pr_completed_days[0], 0.3);
  EXPECT_DOUBLE_EQ(sc_therapy->pr_completed_days[1], 0.4);
  EXPECT_DOUBLE_EQ(sc_therapy->pr_completed_days[2], 0.3);
}

TEST_F(TherapyBuilderTest, BuildComplexTherapy) {
  // Create a complex YAML node representing a multi-therapy protocol
  YAML::Node node;
  node["therapy_ids"].push_back(1);
  node["therapy_ids"].push_back(2);
  node["regimen"].push_back(0);  // Start therapy 1 on day 0
  node["regimen"].push_back(7);  // Start therapy 2 on day 7
  
  // Build the therapy
  std::unique_ptr<Therapy> therapy = TherapyBuilder::build(node, 7);
  
  // Check that we got a MACTherapy
  MACTherapy* mac_therapy = dynamic_cast<MACTherapy*>(therapy.get());
  ASSERT_NE(mac_therapy, nullptr);
  
  // Check properties
  EXPECT_EQ(mac_therapy->get_id(), 7);
  ASSERT_EQ(mac_therapy->get_therapy_ids().size(), 2);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[0], 1);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[1], 2);
  ASSERT_EQ(mac_therapy->get_start_at_days().size(), 2);
  EXPECT_EQ(mac_therapy->get_start_at_days()[0], 0);
  EXPECT_EQ(mac_therapy->get_start_at_days()[1], 7);
}

TEST_F(TherapyBuilderTest, BuildWithInvalidType) {
  // Create an invalid YAML node with neither drug_ids nor therapy_ids
  YAML::Node node;
  node["invalid_field"] = "value";
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 8), std::invalid_argument);
}

TEST_F(TherapyBuilderTest, BuildSimpleWithoutDrugs) {
  // Create a YAML node with drug_ids but no actual drugs
  YAML::Node node;
  node["drug_ids"] = YAML::Node(YAML::NodeType::Sequence);  // Empty sequence
  node["dosing_days"].push_back(3);
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 9), std::invalid_argument);
}

TEST_F(TherapyBuilderTest, BuildSimpleWithoutDosingDays) {
  // Create a YAML node with drug_ids but no dosing days
  YAML::Node node;
  node["drug_ids"].push_back(1);  // Drug1
  node["dosing_days"] = YAML::Node(YAML::NodeType::Sequence);  // Empty sequence
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 10), std::invalid_argument);
}

TEST_F(TherapyBuilderTest, BuildSimpleWithInvalidComplianceRates) {
  // Create a YAML node with incorrect number of pr_completed_days
  YAML::Node node;
  node["drug_ids"].push_back(1);  // Drug1
  node["dosing_days"].push_back(3);  // Max dosing day is 3
  // Only provide 2 compliance rates when 3 are needed
  node["pr_completed_days"].push_back(0.5);
  node["pr_completed_days"].push_back(0.5);
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 11), std::invalid_argument);
}

TEST_F(TherapyBuilderTest, BuildSimpleWithInvalidComplianceTotals) {
  // Create a YAML node where pr_completed_days don't sum to 1
  YAML::Node node;
  node["drug_ids"].push_back(1);  // Drug1
  node["dosing_days"].push_back(2);  // Max dosing day is 2
  // Compliance rates sum to 0.9, not 1.0
  node["pr_completed_days"].push_back(0.4);
  node["pr_completed_days"].push_back(0.5);
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 12), std::invalid_argument);
}

TEST_F(TherapyBuilderTest, BuildComplexWithoutTherapies) {
  // Create a YAML node with therapy_ids but no actual therapies
  YAML::Node node;
  node["therapy_ids"] = YAML::Node(YAML::NodeType::Sequence);  // Empty sequence
  node["regimen"].push_back(0);
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 13), std::invalid_argument);
}

TEST_F(TherapyBuilderTest, BuildComplexWithoutRegimen) {
  // Create a YAML node with therapy_ids but no regimen
  YAML::Node node;
  node["therapy_ids"].push_back(1);
  node["regimen"] = YAML::Node(YAML::NodeType::Sequence);  // Empty sequence
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 14), std::invalid_argument);
}

TEST_F(TherapyBuilderTest, BuildComplexWithMismatchedSizes) {
  // Create a YAML node where therapy_ids and regimen have different sizes
  YAML::Node node;
  node["therapy_ids"].push_back(1);
  node["therapy_ids"].push_back(2);
  node["regimen"].push_back(0);  // Only one regimen entry for two therapies
  
  // Attempt to build should throw an exception
  EXPECT_THROW(TherapyBuilder::build(node, 15), std::invalid_argument);
}
