#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/ParasiteDensity/ParasiteDensityUpdateFunction.h"
#include "Population/SingleHostClonalParasitePopulations.h"

class MockParasiteDensityUpdateFunction : public ParasiteDensityUpdateFunction {
public:
  MOCK_METHOD(double, get_current_parasite_density,
              (ClonalParasitePopulation * parasite, int duration), (override));
};

// mock SingleHostClonalParasitePopulations
class MockSingleHostClonalParasitePopulations : public SingleHostClonalParasitePopulations {
public:
  MockSingleHostClonalParasitePopulations() : SingleHostClonalParasitePopulations(nullptr) {}
  MOCK_METHOD(int, latest_update_time, (), (const, override));
};

class ClonalParasitePopulationTest : public ::testing::Test {
protected:
  void SetUp() override {
    genotype = std::make_unique<Genotype>("abcdef");
    parasite_population = std::make_unique<MockSingleHostClonalParasitePopulations>();
    cpp = std::make_unique<ClonalParasitePopulation>(genotype.get());
    cpp->set_parasite_population(parasite_population.get());
  }

  void TearDown() override {
    cpp.reset();
    genotype.reset();
    parasite_population.reset();
  }

  std::unique_ptr<Genotype> genotype;
  std::unique_ptr<MockSingleHostClonalParasitePopulations> parasite_population;
  std::unique_ptr<ClonalParasitePopulation> cpp;
};

TEST_F(ClonalParasitePopulationTest, Initialization) {
  EXPECT_EQ(cpp->genotype(), genotype.get());
  EXPECT_EQ(cpp->parasite_population(), parasite_population.get());
  EXPECT_EQ(cpp->last_update_log10_parasite_density(),
            ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY);
  EXPECT_EQ(cpp->gametocyte_level(), 0.0);
  EXPECT_EQ(cpp->first_date_in_blood(), -1);
}

TEST_F(ClonalParasitePopulationTest, SetAndGetValues) {
  cpp->set_last_update_log10_parasite_density(5.0);
  EXPECT_EQ(cpp->last_update_log10_parasite_density(), 5.0);

  cpp->set_gametocyte_level(0.5);
  EXPECT_EQ(cpp->gametocyte_level(), 0.5);

  cpp->set_first_date_in_blood(10);
  EXPECT_EQ(cpp->first_date_in_blood(), 10);
}

TEST_F(ClonalParasitePopulationTest, GetLog10InfectiousDensity) {
  // When parasite density is LOG_ZERO_PARASITE_DENSITY
  cpp->set_last_update_log10_parasite_density(ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY);
  cpp->set_gametocyte_level(0.5);
  EXPECT_EQ(cpp->get_log10_infectious_density(),
            ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY);

  // When gametocyte level is 0
  cpp->set_last_update_log10_parasite_density(5.0);
  cpp->set_gametocyte_level(0.0);
  EXPECT_EQ(cpp->get_log10_infectious_density(),
            ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY);

  // Normal case
  cpp->set_last_update_log10_parasite_density(5.0);
  cpp->set_gametocyte_level(0.5);
  EXPECT_DOUBLE_EQ(cpp->get_log10_infectious_density(), 4.6989700043360187);  // 5.0 + log10(0.5)
}

TEST_F(ClonalParasitePopulationTest, PerformDrugAction) {
  const double log10_parasite_density_cured = -1111;
  cpp->set_last_update_log10_parasite_density(5.0);

  // Test with 50% parasite removal
  cpp->perform_drug_action(0.5, log10_parasite_density_cured);
  EXPECT_DOUBLE_EQ(cpp->last_update_log10_parasite_density(),
                   4.6989700043360187);  // 5.0 + log10(0.5)

  // Test with 100% parasite removal
  cpp->perform_drug_action(1.0, log10_parasite_density_cured);
  EXPECT_DOUBLE_EQ(cpp->last_update_log10_parasite_density(), log10_parasite_density_cured);

  // Test with more than 100% parasite removal
  cpp->set_last_update_log10_parasite_density(5.0);
  cpp->perform_drug_action(1.5, log10_parasite_density_cured);
  EXPECT_DOUBLE_EQ(cpp->last_update_log10_parasite_density(), log10_parasite_density_cured);
}

TEST_F(ClonalParasitePopulationTest, UpdateFunction) {
  auto update_function = std::make_unique<MockParasiteDensityUpdateFunction>();
  cpp->set_update_function(update_function.get());
  EXPECT_EQ(cpp->update_function(), update_function.get());
}

TEST_F(ClonalParasitePopulationTest, GetCurrentParasiteDensityWithUpdateFunction) {
  // test case: duration is 5
  auto update_function = std::make_unique<MockParasiteDensityUpdateFunction>();
  cpp->set_update_function(update_function.get());

  // Set up expectations
  EXPECT_CALL(*static_cast<MockSingleHostClonalParasitePopulations*>(parasite_population.get()),
              latest_update_time())
      .WillOnce(testing::Return(6));

  EXPECT_CALL(*update_function.get(), get_current_parasite_density(cpp.get(), 5))
      .WillOnce(testing::Return(3.0));

  // Set initial values
  cpp->set_last_update_log10_parasite_density(2.0);

  // Test the function
  double result = cpp->get_current_parasite_density(11);
  // expect the result is 3.0 which is the return value of the update function
  EXPECT_DOUBLE_EQ(result, 3.0);

}

TEST_F(ClonalParasitePopulationTest, GetCurrentParasiteDensityWithoutUpdateFunction) {
  cpp->set_last_update_log10_parasite_density(2.0);
  double result = cpp->get_current_parasite_density(10);
  EXPECT_DOUBLE_EQ(result, 2.0);
}

TEST_F(ClonalParasitePopulationTest, GetCurrentParasiteDensityZeroDuration) {
  // test case: duration is 0
  auto update_function = std::make_unique<MockParasiteDensityUpdateFunction>();
  cpp->set_update_function(update_function.get());
  cpp->set_last_update_log10_parasite_density(2.0);

  // Set up expectations
  EXPECT_CALL(*static_cast<MockSingleHostClonalParasitePopulations*>(parasite_population.get()),
              latest_update_time())
      .WillOnce(testing::Return(5));

  double result = cpp->get_current_parasite_density(5);
  EXPECT_DOUBLE_EQ(result, 2.0);
}

// Edge case tests
TEST_F(ClonalParasitePopulationTest, GetCurrentParasiteDensityNegativeDuration) {
  auto update_function = std::make_unique<MockParasiteDensityUpdateFunction>();
  cpp->set_update_function(update_function.get());
  cpp->set_last_update_log10_parasite_density(2.0);

  // Set up expectations for negative duration
  EXPECT_CALL(*static_cast<MockSingleHostClonalParasitePopulations*>(parasite_population.get()),
              latest_update_time())
      .WillOnce(testing::Return(10));

  // expected to throw an exception
  EXPECT_THROW(cpp->get_current_parasite_density(5), std::invalid_argument);
}

TEST_F(ClonalParasitePopulationTest, GetLog10InfectiousDensityEdgeCases) {
  // Test with very small parasite density
  cpp->set_last_update_log10_parasite_density(-999.9);
  cpp->set_gametocyte_level(0.5);
  EXPECT_DOUBLE_EQ(cpp->get_log10_infectious_density(), -999.9 + log10(0.5));

  // Test with very high parasite density
  cpp->set_last_update_log10_parasite_density(1000.0);
  cpp->set_gametocyte_level(0.5);
  EXPECT_DOUBLE_EQ(cpp->get_log10_infectious_density(), 1000.0 + log10(0.5));

  // Test with very small gametocyte level
  cpp->set_last_update_log10_parasite_density(5.0);
  cpp->set_gametocyte_level(1e-10);
  EXPECT_DOUBLE_EQ(cpp->get_log10_infectious_density(), 5.0 + log10(1e-10));
}

TEST_F(ClonalParasitePopulationTest, PerformDrugActionEdgeCases) {
  const double log10_parasite_density_cured = -1111;

  // Test with very small parasite removal
  cpp->set_last_update_log10_parasite_density(5.0);
  auto percent_parasite_remove = 0.0001;
  cpp->perform_drug_action(percent_parasite_remove, log10_parasite_density_cured);
  EXPECT_NEAR(cpp->last_update_log10_parasite_density(), 5.0 + log10(1 - percent_parasite_remove),
              1e-10);

  // Test with very large parasite removal (but less than 1)
  cpp->set_last_update_log10_parasite_density(5.0);
  percent_parasite_remove = 0.9999;
  cpp->perform_drug_action(percent_parasite_remove, log10_parasite_density_cured);
  EXPECT_NEAR(cpp->last_update_log10_parasite_density(), 5.0 + log10(1 - percent_parasite_remove),
              1e-10);

  // Test with exactly 1.0 parasite removal
  cpp->set_last_update_log10_parasite_density(5.0);
  percent_parasite_remove = 1.0;
  cpp->perform_drug_action(percent_parasite_remove, log10_parasite_density_cured);
  EXPECT_DOUBLE_EQ(cpp->last_update_log10_parasite_density(), log10_parasite_density_cured);

  // Test with negative parasite removal, should throw an exception
  cpp->set_last_update_log10_parasite_density(5.0);
  percent_parasite_remove = -0.5;
  EXPECT_THROW(cpp->perform_drug_action(percent_parasite_remove, log10_parasite_density_cured),
               std::invalid_argument);
}

TEST_F(ClonalParasitePopulationTest, GetCurrentParasiteDensityNullUpdateFunction) {
  // Test with null update function and various parasite densities
  cpp->set_update_function(nullptr);

  // Test with zero parasite density
  cpp->set_last_update_log10_parasite_density(ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY);
  EXPECT_DOUBLE_EQ(cpp->get_current_parasite_density(10),
                   ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY);

  // Test with normal parasite density
  cpp->set_last_update_log10_parasite_density(5.0);
  EXPECT_DOUBLE_EQ(cpp->get_current_parasite_density(10), 5.0);

  // Test with negative parasite density
  cpp->set_last_update_log10_parasite_density(-5.0);
  EXPECT_DOUBLE_EQ(cpp->get_current_parasite_density(10), -5.0);
}

