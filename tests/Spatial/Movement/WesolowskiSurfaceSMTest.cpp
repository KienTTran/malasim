#include <gtest/gtest.h>
#include <memory>

#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"
#include "Spatial/Movement/WesolowskiSurfaceSM.h"
#include "Utils/TypeDef.h"

class WesolowskiSurfaceSMTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    
    // This test assumes 2 locations, recreate rasters with only 2 locations
    test_fixtures::create_test_raster_2_locations("test_init_pop.asc", 1000.0);
    test_fixtures::create_test_raster_2_locations("test_beta.asc", 0.5);
    test_fixtures::create_test_raster_2_locations("test_treatment.asc", 0.6);
    test_fixtures::create_test_raster_2_locations("test_ecozone.asc", 1.0);
    test_fixtures::create_test_raster_2_locations("test_travel.asc", 0.2);
    
    // Initialize Model configuration
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create a WesolowskiSurfaceSM with specific parameters
    kappa = 2.0;
    alpha = 0.1;
    beta = 0.2;
    gamma = 0.3;
    number_of_locations = 2;
    model = std::make_unique<Spatial::WesolowskiSurfaceSM>(kappa, alpha, beta, gamma, number_of_locations);
    
    // Set up travel surface for testing
    // This would normally be populated in the prepare() method
    model->travel = {0.2, 0.5};
  }

  void TearDown() override {
    model.reset();
    test_fixtures::cleanup_test_files();
  }

  // Test parameters
  double kappa;
  double alpha;
  double beta;
  double gamma;
  int number_of_locations;
  std::unique_ptr<Spatial::WesolowskiSurfaceSM> model;
};

TEST_F(WesolowskiSurfaceSMTest, InitializeCorrectly) {
  // Check that parameters are correctly initialized
  EXPECT_DOUBLE_EQ(model->get_kappa(), kappa);
  EXPECT_DOUBLE_EQ(model->get_alpha(), alpha);
  EXPECT_DOUBLE_EQ(model->get_beta(), beta);
  EXPECT_DOUBLE_EQ(model->get_gamma(), gamma);
}

TEST_F(WesolowskiSurfaceSMTest, PrepareMethodWorks) {
  // Reset the travel surface
  model->travel.clear();
  
  // Verify prepare() method populates the travel surface
  EXPECT_NO_THROW(model->prepare());
  
  // After prepare, travel should be populated
  EXPECT_FALSE(model->travel.empty());
  EXPECT_EQ(model->travel.size(), number_of_locations);
}

TEST_F(WesolowskiSurfaceSMTest, CalculateMovementToSameLocation) {
  // Movement to same location should be zero
  const int from_location = 0;
  const std::vector<double> relative_distance_vector = {0.0, 10.0};
  const std::vector<int> residents_by_location = {1000, 2000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Movement to same location (distance = 0) should be 0
  EXPECT_DOUBLE_EQ(movement[0], 0.0);
}

TEST_F(WesolowskiSurfaceSMTest, CalculateMovementPattern) {
  // Test the movement pattern calculation
  const int from_location = 0;
  const std::vector<double> relative_distance_vector = {0.0, 10.0};
  const std::vector<int> residents_by_location = {1000, 2000};
  
  auto movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Verify movement pattern follows the gravity model with travel surface penalty: 
  // Basic gravity model: probability = κ * (pop_i^α * pop_j^β) / d(i,j)^γ
  // With travel surface: result = probability / (1 + t_i + t_j)
  
  double probability = kappa * 
                     (pow(residents_by_location[from_location], alpha) * 
                      pow(residents_by_location[1], beta)) / 
                     pow(relative_distance_vector[1], gamma);
                     
  double expected_movement = probability / (1 + model->travel[from_location] + model->travel[1]);
  
  EXPECT_NEAR(movement[1], expected_movement, 1e-10);
}

TEST_F(WesolowskiSurfaceSMTest, TravelSurfacePenalty) {
  // Test that travel surface properly penalizes movement
  const int from_location = 0;
  const std::vector<double> relative_distance_vector = {0.0, 10.0};
  const std::vector<int> residents_by_location = {1000, 2000};
  
  // Calculate movement with current travel surface
  auto original_movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Store original travel values
  auto original_travel = model->travel;
  
  // Increase travel penalties
  model->travel = {0.4, 1.0};  // Double the original values
  
  // Calculate movement with increased travel penalties
  auto penalized_movement = model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location);
  
  // Movement should decrease with higher travel penalties
  EXPECT_LT(penalized_movement[1], original_movement[1]);
  
  // Restore original travel values
  model->travel = original_travel;
}

TEST_F(WesolowskiSurfaceSMTest, EmptyTravelSurfaceThrows) {
  // Empty travel surface should throw an exception
  const int from_location = 0;
  const std::vector<double> relative_distance_vector = {0.0, 10.0};
  const std::vector<int> residents_by_location = {1000, 2000};
  
  // Clear the travel surface
  model->travel.clear();
  
  // Trying to calculate movement should throw an exception
  EXPECT_THROW(
    model->get_v_relative_out_movement_to_destination(
      from_location, number_of_locations, relative_distance_vector, residents_by_location),
    std::runtime_error
  );
}
