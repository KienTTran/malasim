#include <gtest/gtest.h>

#include "Events/Population/ImportationPeriodicallyEvent.h"
#include "MDC/ModelDataCollector.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "fixtures/TestFileGenerators.h"

class ImportationPeriodicallyExecutionTest : public ::testing::Test {
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

TEST_F(ImportationPeriodicallyExecutionTest, ImportsParasitesIntoSusceptiblePopulation) {
  Model::get_random()->set_seed(42);
  for (int i = 0; i < 100; ++i) {
    ASSERT_NE(Model::get_population()->give_1_birth(0), nullptr);
  }
  Model::get_mdc()->popsize_by_location_hoststate()[0][Person::SUSCEPTIBLE] = 100;

  ImportationPeriodicallyEvent event(0, 1, 0, 20, 0);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());

  EXPECT_FALSE(event.is_executable());
  const auto &index = Model::get_population()
                          ->get_person_index<PersonIndexByLocationStateAgeClass>()
                          ->vPerson();
  std::size_t asymptomatic = 0;
  for (const auto &age_class : index[0][Person::ASYMPTOMATIC]) {
    asymptomatic += age_class.size();
  }
  EXPECT_GT(asymptomatic, 0U);
}
