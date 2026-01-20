#include <gtest/gtest.h>
#include <memory>
#include <yaml-cpp/yaml.h>

#include "Treatment/Strategies/StrategyBuilder.h"
#include "Treatment/Strategies/IStrategy.h"
#include "Treatment/Strategies/SFTStrategy.h"
#include "Treatment/Strategies/MFTStrategy.h"
#include "Treatment/Strategies/CyclingStrategy.h"
#include "Treatment/Strategies/AdaptiveCyclingStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "Treatment/Therapies/TherapyBuilder.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"

class StrategyBuilderTest : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Use therapies from the therapy database
    therapies.clear();
    // Need at least 2 therapies
    for (int i = 0; i < 2 && i < Model::get_therapy_db().size(); i++) {
      therapies.push_back(Model::get_therapy_db()[i].get());
    }
    
    // Skip test if not enough therapies
    if (therapies.size() < 2) {
      GTEST_SKIP() << "Not enough therapies in the database to run this test";
    }
  }

  void TearDown() override {
    therapies.clear();
  }

  std::vector<Therapy*> therapies;
  
  // Helper to create a YAML node for strategy testing
  YAML::Node create_sft_strategy_node() {
    YAML::Node node;
    node["name"] = "Test SFT Strategy";
    node["type"] = "SFT";
    
    YAML::Node therapy_ids;
    therapy_ids.push_back(therapies[0]->get_id());
    node["therapy_ids"] = therapy_ids;
    
    return node;
  }
  
  YAML::Node create_mft_strategy_node() {
    YAML::Node node;
    node["name"] = "Test MFT Strategy";
    node["type"] = "MFT";
    
    YAML::Node therapy_ids;
    therapy_ids.push_back(therapies[0]->get_id());
    therapy_ids.push_back(therapies[1]->get_id());
    node["therapy_ids"] = therapy_ids;
    
    YAML::Node distributions;
    distributions.push_back(0.3);
    distributions.push_back(0.7);
    node["distributions"] = distributions;
    
    return node;
  }
  
  YAML::Node create_cycling_strategy_node() {
    YAML::Node node;
    node["name"] = "Test Cycling Strategy";
    node["type"] = "Cycling";
    
    YAML::Node therapy_ids;
    therapy_ids.push_back(therapies[0]->get_id());
    therapy_ids.push_back(therapies[1]->get_id());
    node["therapy_ids"] = therapy_ids;
    
    node["cycling_time"] = 30;
    
    return node;
  }
  
  YAML::Node create_adaptive_cycling_strategy_node() {
    YAML::Node node;
    node["name"] = "Test Adaptive Cycling Strategy";
    node["type"] = "AdaptiveCycling";
    
    YAML::Node therapy_ids;
    therapy_ids.push_back(therapies[0]->get_id());
    therapy_ids.push_back(therapies[1]->get_id());
    node["therapy_ids"] = therapy_ids;
    
    node["trigger_value"] = 0.15;
    node["delay_until_actual_trigger"] = 30;
    node["turn_off_days"] = 90;
    
    return node;
  }
};

TEST_F(StrategyBuilderTest, BuildSFTStrategy) {
  YAML::Node node = create_sft_strategy_node();
  
  // Build the strategy
  std::unique_ptr<IStrategy> strategy = StrategyBuilder::build(node, 1);
  
  // Check that we got the right type
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::SFT);
  
  // Check properties
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "Test SFT Strategy");
  
  // Cast to SFTStrategy and check therapy list
  auto sft_strategy = dynamic_cast<SFTStrategy*>(strategy.get());
  ASSERT_NE(sft_strategy, nullptr);
  ASSERT_EQ(sft_strategy->get_therapy_list().size(), 1);
  EXPECT_EQ(sft_strategy->get_therapy_list()[0]->get_id(), therapies[0]->get_id());
}

TEST_F(StrategyBuilderTest, BuildMFTStrategy) {
  YAML::Node node = create_mft_strategy_node();
  
  // Build the strategy
  std::unique_ptr<IStrategy> strategy = StrategyBuilder::build(node, 2);
  
  // Check that we got the right type
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::MFT);
  
  // Check properties
  EXPECT_EQ(strategy->id, 2);
  EXPECT_EQ(strategy->name, "Test MFT Strategy");
  
  // Cast to MFTStrategy and check therapy list and distributions
  auto mft_strategy = dynamic_cast<MFTStrategy*>(strategy.get());
  ASSERT_NE(mft_strategy, nullptr);
  
  ASSERT_EQ(mft_strategy->therapy_list.size(), 2);
  EXPECT_EQ(mft_strategy->therapy_list[0]->get_id(), therapies[0]->get_id());
  EXPECT_EQ(mft_strategy->therapy_list[1]->get_id(), therapies[1]->get_id());
  
  ASSERT_EQ(mft_strategy->distribution.size(), 2);
  EXPECT_DOUBLE_EQ(mft_strategy->distribution[0], 0.3);
  EXPECT_DOUBLE_EQ(mft_strategy->distribution[1], 0.7);
}

TEST_F(StrategyBuilderTest, BuildCyclingStrategy) {
  YAML::Node node = create_cycling_strategy_node();
  
  // Build the strategy
  std::unique_ptr<IStrategy> strategy = StrategyBuilder::build(node, 3);
  
  // Check that we got the right type
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::Cycling);
  
  // Check properties
  EXPECT_EQ(strategy->id, 3);
  EXPECT_EQ(strategy->name, "Test Cycling Strategy");
  
  // Cast to CyclingStrategy and check therapy list and cycling time
  auto cycling_strategy = dynamic_cast<CyclingStrategy*>(strategy.get());
  ASSERT_NE(cycling_strategy, nullptr);
  
  ASSERT_EQ(cycling_strategy->therapy_list.size(), 2);
  EXPECT_EQ(cycling_strategy->therapy_list[0]->get_id(), therapies[0]->get_id());
  EXPECT_EQ(cycling_strategy->therapy_list[1]->get_id(), therapies[1]->get_id());
  
  EXPECT_EQ(cycling_strategy->cycling_time, 30);
}

TEST_F(StrategyBuilderTest, BuildAdaptiveCyclingStrategy) {
  YAML::Node node = create_adaptive_cycling_strategy_node();
  
  // Build the strategy
  std::unique_ptr<IStrategy> strategy = StrategyBuilder::build(node, 4);
  
  // Check that we got the right type
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::AdaptiveCycling);
  
  // Check properties
  EXPECT_EQ(strategy->id, 4);
  EXPECT_EQ(strategy->name, "Test Adaptive Cycling Strategy");
  
  // Cast to AdaptiveCyclingStrategy and check therapy list and parameters
  auto adaptive_cycling_strategy = dynamic_cast<AdaptiveCyclingStrategy*>(strategy.get());
  ASSERT_NE(adaptive_cycling_strategy, nullptr);
  
  ASSERT_EQ(adaptive_cycling_strategy->therapy_list.size(), 2);
  EXPECT_EQ(adaptive_cycling_strategy->therapy_list[0]->get_id(), therapies[0]->get_id());
  EXPECT_EQ(adaptive_cycling_strategy->therapy_list[1]->get_id(), therapies[1]->get_id());
  
  EXPECT_DOUBLE_EQ(adaptive_cycling_strategy->trigger_value, 0.15);
  EXPECT_EQ(adaptive_cycling_strategy->delay_until_actual_trigger, 30);
  EXPECT_EQ(adaptive_cycling_strategy->turn_off_days, 90);
}

TEST_F(StrategyBuilderTest, AddTherapies) {
  YAML::Node node;
  YAML::Node therapy_ids;
  therapy_ids.push_back(therapies[0]->get_id());
  therapy_ids.push_back(therapies[1]->get_id());
  node["therapy_ids"] = therapy_ids;
  
  // Create a strategy to add therapies to
  auto strategy = std::make_unique<SFTStrategy>();
  
  // Add therapies from the node
  StrategyBuilder::add_therapies(node, strategy.get());
  
  // Check therapies were added correctly
  ASSERT_EQ(strategy->get_therapy_list().size(), 2);
  EXPECT_EQ(strategy->get_therapy_list()[0]->get_id(), therapies[0]->get_id());
  EXPECT_EQ(strategy->get_therapy_list()[1]->get_id(), therapies[1]->get_id());
}

TEST_F(StrategyBuilderTest, AddDistributions) {
  YAML::Node node;
  YAML::Node distributions;
  distributions.push_back(0.2);
  distributions.push_back(0.3);
  distributions.push_back(0.5);
  node["distributions"] = distributions;
  
  // Create vector to add distributions to
  std::vector<double> dist_vector;
  
  // Add distributions from the node - must pass the sequence node, not the parent node
  StrategyBuilder::add_distributions(node["distributions"], dist_vector);
  
  // Check distributions were added correctly
  ASSERT_EQ(dist_vector.size(), 3);
  EXPECT_DOUBLE_EQ(dist_vector[0], 0.2);
  EXPECT_DOUBLE_EQ(dist_vector[1], 0.3);
  EXPECT_DOUBLE_EQ(dist_vector[2], 0.5);
}

TEST_F(StrategyBuilderTest, InvalidStrategyType) {
  YAML::Node node;
  node["name"] = "Invalid Strategy";
  node["type"] = "InvalidType";
  
  // Attempt to build with invalid type should throw
  EXPECT_THROW(StrategyBuilder::build(node, 5), std::runtime_error);
}

TEST_F(StrategyBuilderTest, MissingTherapies) {
  YAML::Node node;
  node["name"] = "Missing Therapies";
  node["type"] = "SFT";
  
  // No therapy_ids specified
  EXPECT_THROW(StrategyBuilder::build(node, 6), std::runtime_error);
}
