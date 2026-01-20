#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "Spatial/Movement/BurkinaFasoSM.h"
#include "Utils/TypeDef.h"

class BurkinaFasoSMTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize Model configuration
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
    
    // Set up test parameters
    tau = 0.5;
    alpha = 0.3;
    rho = 0.2;
    capital = 0.4;
    penalty = 0.6;
    number_of_locations = 3;
    
    // Create a simple distance matrix for testing
    spatial_distance_matrix = {
      {0.0, 10.0, 20.0},
      {10.0, 0.0, 15.0},
      {20.0, 15.0, 0.0}
    };
    
    // Create the model
    model = std::make_unique<Spatial::BurkinaFasoSM>(
        tau, alpha, rho, capital, penalty, number_of_locations, spatial_distance_matrix);
    
    // Prepare the model
    model->prepare();
  }

  void TearDown() override {
    model.reset();
  }

  // Test parameters
  double tau;
  double alpha;
  double rho;
  double capital;
  double penalty;
  int number_of_locations;
  std::vector<std::vector<double>> spatial_distance_matrix;
  std::unique_ptr<Spatial::BurkinaFasoSM> model;
};

TEST_F(BurkinaFasoSMTest, InitializeCorrectly) {
  // Check that parameters are correctly initialized
  EXPECT_DOUBLE_EQ(model->get_tau(), tau);
  EXPECT_DOUBLE_EQ(model->get_alpha(), alpha);
  EXPECT_DOUBLE_EQ(model->get_rho(), rho);
  EXPECT_DOUBLE_EQ(model->get_capital(), capital);
  EXPECT_DOUBLE_EQ(model->get_penalty(), penalty);
}

TEST_F(BurkinaFasoSMTest, PrepareMethodWorks) {
  // Verify prepare() method doesn't throw exceptions
  EXPECT_NO_THROW(model->prepare());
}

TEST_F(BurkinaFasoSMTest, CalculateMovementToSameLocation) {
  // Test that movement to same location is zero
  const int from_location = 0;
  const std::vector<double> relative_distance_vector = {0.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 2000, 3000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Movement to same location should be 0
  EXPECT_DOUBLE_EQ(movement[0], 0.0);
}

TEST_F(BurkinaFasoSMTest, CalculateMovementPattern) {
  // Test the movement pattern calculation
  const int from_location = 0;
  const std::vector<double> relative_distance_vector = {0.0, 10.0, 20.0};
  const std::vector<int> residents_by_location = {1000, 2000, 3000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Movement vector should have correct size
  EXPECT_EQ(movement.size(), number_of_locations);
  
  // Movement values should be non-negative
  for (const auto &value : movement) {
    EXPECT_GE(value, 0.0);
  }
}

TEST_F(BurkinaFasoSMTest, UpdateParameters) {
  // Test parameter update methods
  const double new_tau = 1.5;
  const double new_alpha = 1.3;
  const double new_rho = 1.2;
  const double new_capital = 1.4;
  const double new_penalty = 1.6;
  
  model->set_tau(new_tau);
  model->set_alpha(new_alpha);
  model->set_rho(new_rho);
  model->set_capital(new_capital);
  model->set_penalty(new_penalty);
  
  EXPECT_DOUBLE_EQ(model->get_tau(), new_tau);
  EXPECT_DOUBLE_EQ(model->get_alpha(), new_alpha);
  EXPECT_DOUBLE_EQ(model->get_rho(), new_rho);
  EXPECT_DOUBLE_EQ(model->get_capital(), new_capital);
  EXPECT_DOUBLE_EQ(model->get_penalty(), new_penalty);
  
  // After parameter updates, prepare needs to be called again
  EXPECT_NO_THROW(model->prepare());
}
