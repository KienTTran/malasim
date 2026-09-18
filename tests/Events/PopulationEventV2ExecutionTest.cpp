#include <gtest/gtest.h>

#include "Events/Population/IntroduceParasitesPeriodicallyEventV2.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventV2ExecutionTest : public ::testing::Test {
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

TEST_F(PopulationEventV2ExecutionTest, ExecutesWithoutCasesAndStopsAtEndDate) {
  IntroduceParasitesPeriodicallyEventV2 event({}, 0, 1, 0, 0, 0);
  event.set_executable(true);
  event.execute();
  EXPECT_FALSE(event.is_executable());
}

TEST_F(PopulationEventV2ExecutionTest, ReschedulesWhileBeforeEndDate) {
  IntroduceParasitesPeriodicallyEventV2 event({}, 0, 1, 0, 0, 10);
  event.set_executable(true);
  event.execute();
  EXPECT_FALSE(event.is_executable());
  EXPECT_FALSE(Model::get_scheduler()->get_world_events().get_events().empty());
}

TEST_F(PopulationEventV2ExecutionTest, DefaultsMissingEndDateToSimulationEnd) {
  IntroduceParasitesPeriodicallyEventV2 event({}, 0, 1, 0, 0, -1);
  EXPECT_EQ(event.end_day, Model::get_config()->get_simulation_timeframe().get_total_time());
}

TEST_F(PopulationEventV2ExecutionTest, SkipsImportWhenPopulationCannotSupplyCases) {
  Model::get_random()->set_seed(42);
  Model::get_mdc()->popsize_by_location_hoststate()[0][0] = 0;
  IntroduceParasitesPeriodicallyEventV2 event({}, 0, 1, 1000, 0, 0);
  event.set_executable(true);
  event.execute();
  EXPECT_FALSE(event.is_executable());
}

TEST_F(PopulationEventV2ExecutionTest, ImportsConfiguredAllelesIntoSusceptiblePopulation) {
  for (int i = 0; i < 200; ++i) {
    ASSERT_NE(Model::get_population()->give_1_birth(0), nullptr);
  }
  Model::get_mdc()->popsize_by_location_hoststate()[0][Person::SUSCEPTIBLE] = 200;
  Model::get_random()->set_seed(42);

  std::vector<std::vector<double>> allele_distributions;
  for (std::size_t locus = 0; locus < Model::get_genotype_db()->get_weight().size(); ++locus) {
    allele_distributions.push_back({1.0});
  }
  IntroduceParasitesPeriodicallyEventV2 event(allele_distributions, 0, 1, 20, 0, 0);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());

  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  std::size_t infected = 0;
  for (const auto &age_class : index->vPerson()[0][Person::ASYMPTOMATIC]) {
    for (auto* person : age_class) {
      infected += person->get_all_clonal_parasite_populations()->size();
    }
  }
  EXPECT_GT(infected, 0U);
}
