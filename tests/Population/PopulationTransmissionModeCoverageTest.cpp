#include <gtest/gtest.h>

#include "Mosquito/Mosquito.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class PopulationTransmissionModeCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node& config) {
      config["transmission_settings"]["transmission_parameter"] = 0.0;
      config["transmission_settings"]["p_infection_from_an_infectious_bite"] = 1.0;
    });
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(PopulationTransmissionModeCoverageTest, UsesConfiguredProbabilityForInfectiousBites) {
  constexpr int location = 0;
  auto* population = Model::get_population();
  auto* mosquito = Model::get_mosquito();
  ASSERT_FALSE(population->all_alive_persons_by_location()[location].empty());
  ASSERT_FALSE(mosquito->genotypes_table[0][location].empty());

  population->force_of_infection_for_n_days_by_location()[0][location] = 1.0;
  Model::get_config()->location_db()[location].beta = 100.0;
  auto* genotype = Model::get_genotype_db()->at(0);
  for (auto& cohort_genotype : mosquito->genotypes_table[0][location]) {
    cohort_genotype = genotype;
  }

  EXPECT_NO_THROW(population->perform_infection_event_at_location(location, 0));
}
