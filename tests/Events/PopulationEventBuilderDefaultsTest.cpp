#include <gtest/gtest.h>

#include "Configuration/Config.h"
#include "Events/Population/PopulationEventBuilder.h"
#include "Events/Population/SingleRoundMDAEvent.h"
#include "Events/Population/TurnOnMutationEvent.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventBuilderDefaultsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
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

TEST_F(PopulationEventBuilderDefaultsTest, UsesConfiguredMutationProbabilityAndPerLocationMdaRates) {
  auto* config = Model::get_config();
  const auto fallback = PopulationEventBuilder::build_turn_on_mutation_event(
      YAML::Load("- date: 2024/01/11"), config);
  ASSERT_EQ(fallback.size(), 1U);
  EXPECT_DOUBLE_EQ(
      dynamic_cast<TurnOnMutationEvent*>(fallback[0].get())->mutation_probability,
      config->get_genotype_parameters().get_mutation_probability_per_locus());

  const auto locations = config->number_of_locations();
  YAML::Node rates;
  rates["date"] = "2024/01/02";
  rates["fraction_population_targeted"] = YAML::Node(YAML::NodeType::Sequence);
  for (std::size_t i = 0; i < locations; ++i) {
    rates["fraction_population_targeted"].push_back(0.1 * static_cast<double>(i + 1));
  }
  rates["days_to_complete_all_treatments"] = 2;
  YAML::Node entries(YAML::NodeType::Sequence);
  entries.push_back(rates);
  const auto mda = PopulationEventBuilder::build_single_round_mda_event(entries, config);
  ASSERT_EQ(mda.size(), 1U);
  const auto* event = dynamic_cast<SingleRoundMDAEvent*>(mda[0].get());
  ASSERT_NE(event, nullptr);
  ASSERT_EQ(event->get_fraction_population_targeted().size(), locations);
  EXPECT_DOUBLE_EQ(event->get_fraction_population_targeted().back(),
                   0.1 * static_cast<double>(locations));
}
