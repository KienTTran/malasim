#include <gtest/gtest.h>

#include "Population/Population.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

TEST(PopulationZeroCaseTest, ReportsNoCasesWhenNoInitialParasitesConfigured) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["genotype_parameters"]["initial_parasite_info"] =
        YAML::Node(YAML::NodeType::Sequence);
  });

  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  ASSERT_TRUE(Model::get_instance()->initialize());

  EXPECT_TRUE(Model::get_population()->has_0_case());

  Model::get_instance()->release();
  test_fixtures::cleanup_test_files();
}
