#include <gtest/gtest.h>

#include <tuple>
#include <vector>

#include "Events/Population/DistrictImportationDailyEvent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class DistrictImportationDailyExecutionTest : public ::testing::Test {
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

TEST_F(DistrictImportationDailyExecutionTest, MutatesParasitesInInfectedDistrictPopulation) {
  Model::get_random()->set_seed(42);
  for (int i = 0; i < 20; ++i) {
    auto* person = Model::get_population()->give_1_birth(1);
    ASSERT_NE(person, nullptr);
    person->set_host_state(Person::ASYMPTOMATIC);
    ASSERT_NE(person->add_new_parasite_to_blood(Model::get_genotype_db()->at(0)), nullptr);
  }

  DistrictImportationDailyEvent event(1, 100.0, 0, {{5, 86, 'Y'}});
  event.set_executable(true);
  event.execute();

  EXPECT_FALSE(event.is_executable());
}
