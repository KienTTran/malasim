#include <gtest/gtest.h>
#include <memory>

#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "Spatial/Movement/WesolowskiSM.hxx"
#include "Utils/TypeDef.h"

class WesolowskiSMTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize Model configuration
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Create a WesolowskiSM with specific parameters
    kappa = 2.0;
    alpha = 0.1;
    beta = 0.2;
    gamma = 0.3;
    model = std::make_unique<Spatial::WesolowskiSM>(kappa, alpha, beta, gamma);
  }

  void TearDown() override {
    model.reset();
  }

  // Test parameters
  double kappa;
  double alpha;
  double beta;
  double gamma;
  std::unique_ptr<Spatial::WesolowskiSM> model;
};

TEST_F(WesolowskiSMTest, InitializeCorrectly) {
  // Check that parameters are correctly initialized
  EXPECT_DOUBLE_EQ(model->get_kappa(), kappa);
  EXPECT_DOUBLE_EQ(model->get_alpha(), alpha);
  EXPECT_DOUBLE_EQ(model->get_beta(), beta);
  EXPECT_DOUBLE_EQ(model->get_gamma(), gamma);
}

TEST_F(WesolowskiSMTest, CalculateMovementToSameLocation) {
  // Movement to same location should be zero
  const int from_location = 0;
  const int number_of_locations = 3;
  const std::vector<double> relative_distance_vector = {0.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 2000, 3000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Movement to same location (distance = 0) should be 0
  EXPECT_DOUBLE_EQ(movement[0], 0.0);
}

TEST_F(WesolowskiSMTest, CalculateMovementPattern) {
  // Test the movement pattern calculation
  const int from_location = 0;
  const int number_of_locations = 3;
  const std::vector<double> relative_distance_vector = {0.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 2000, 3000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Verify movement pattern follows the gravity model formula: 
  // N_{ij} = κ * (pop_i^α * pop_j^β) / d(i,j)^γ
  double expected_movement1 = kappa * 
                             (pow(residents_by_location[from_location], alpha) * 
                              pow(residents_by_location[1], beta)) / 
                             pow(relative_distance_vector[1], gamma);
  
  double expected_movement2 = kappa * 
                             (pow(residents_by_location[from_location], alpha) * 
                              pow(residents_by_location[2], beta)) / 
                             pow(relative_distance_vector[2], gamma);
  
  EXPECT_NEAR(movement[1], expected_movement1, 1e-10);
  EXPECT_NEAR(movement[2], expected_movement2, 1e-10);
}

TEST_F(WesolowskiSMTest, VerifyParameterEffects) {
  // Test that changing parameters affects the movement pattern as expected
  const int from_location = 0;
  const int number_of_locations = 3;
  const std::vector<double> relative_distance_vector = {0.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 2000, 3000};
  
  // Get baseline movement
  auto baseline_movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Increase kappa and verify movement increases proportionally
  double new_kappa = kappa * 2.0;
  model->set_kappa(new_kappa);
  
  auto new_movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Movement should be doubled when kappa is doubled
  EXPECT_NEAR(new_movement[1], baseline_movement[1] * 2.0, 1e-10);
  EXPECT_NEAR(new_movement[2], baseline_movement[2] * 2.0, 1e-10);
  
  // Reset kappa to original value
  model->set_kappa(kappa);
}

TEST_F(WesolowskiSMTest, DistanceEffectsOnMovement) {
  // Movement probability should decrease with distance
  const int from_location = 0;
  const int number_of_locations = 4;
  const std::vector<double> relative_distance_vector = {0.0, 5.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 1000, 1000, 1000};  // Same population for all
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // With identical populations, movement probability should decrease with distance
  EXPECT_GT(movement[1], movement[2]);  // 5.0 < 10.0
  EXPECT_GT(movement[2], movement[3]);  // 10.0 < 20.0
}

TEST_F(WesolowskiSMTest, PopulationEffectsOnMovement) {
  // Movement probability should increase with destination population
  const int from_location = 0;
  const int number_of_locations = 4;
  const std::vector<double> relative_distance_vector = {0.0, 10.0, 10.0, 10.0};  // Same distance
  const std::vector<int> residents_by_location = {1000, 1000, 2000, 4000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // With identical distances, movement probability should increase with population
  EXPECT_GT(movement[2], movement[1]);  // 2000 > 1000
  EXPECT_GT(movement[3], movement[2]);  // 4000 > 2000
}
