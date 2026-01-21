#include <gtest/gtest.h>
#include <memory>

#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"
#include "Spatial/Movement/BarabasiSM.hxx"
#include "Utils/TypeDef.h"

class BarabasiSMTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    // Initialize Model configuration
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create a BarabasiSM with specific parameters
    r_g_0 = 1.0;
    beta_r = 0.5;
    kappa = 3.0;
    model = std::make_unique<Spatial::BarabasiSM>(r_g_0, beta_r, kappa);
  }

  void TearDown() override {
    model.reset();
    test_fixtures::cleanup_test_files();
  }

  // Test parameters
  double r_g_0;
  double beta_r;
  double kappa;
  std::unique_ptr<Spatial::BarabasiSM> model;
};

TEST_F(BarabasiSMTest, InitializeCorrectly) {
  // Check that parameters are correctly initialized
  EXPECT_DOUBLE_EQ(model->get_r_g_0(), r_g_0);
  EXPECT_DOUBLE_EQ(model->get_beta_r(), beta_r);
  EXPECT_DOUBLE_EQ(model->get_kappa(), kappa);
}

TEST_F(BarabasiSMTest, CalculateMovementToSameLocation) {
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

TEST_F(BarabasiSMTest, CalculateMovementPattern) {
  // Test the movement pattern calculation
  const int from_location = 0;
  const int number_of_locations = 3;
  const std::vector<double> relative_distance_vector = {0.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 2000, 3000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Verify movement pattern follows the formula: P(r_g) = (r_g + r_g^0)^{-\beta_r}exp(-r_g/\kappa)
  double expected_movement1 = pow((relative_distance_vector[1] + r_g_0), -beta_r) * 
                             exp(-relative_distance_vector[1] / kappa);
  double expected_movement2 = pow((relative_distance_vector[2] + r_g_0), -beta_r) * 
                             exp(-relative_distance_vector[2] / kappa);
  
  EXPECT_NEAR(movement[1], expected_movement1, 1e-10);
  EXPECT_NEAR(movement[2], expected_movement2, 1e-10);
}

TEST_F(BarabasiSMTest, VerifyDecreasingPattern) {
  // Movement probability should decrease with distance
  const int from_location = 0;
  const int number_of_locations = 4;
  const std::vector<double> relative_distance_vector = {0.0, 5.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 2000, 3000, 4000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Movement probability should decrease with distance
  EXPECT_GT(movement[1], movement[2]);
  EXPECT_GT(movement[2], movement[3]);
}
