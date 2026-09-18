#include <gtest/gtest.h>

#include <tuple>
#include <vector>

#include "Events/Population/DistrictImportationDailyEvent.h"
#include "Events/Population/ImportationEvent.h"
#include "Events/Population/ImportationPeriodicallyEvent.h"
#include "Events/Population/ImportationPeriodicallyRandomEvent.h"
#include "Events/Population/Introduce580YMutantEvent.h"
#include "Events/Population/IntroduceAmodiaquineMutantEvent.h"
#include "Events/Population/IntroduceLumefantrineMutantEvent.h"
#include "Events/Population/IntroducePlas2CopyParasiteEvent.h"
#include "Events/Population/IntroduceTripleMutantToDPMEvent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventExecutionTest : public ::testing::Test {
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

TEST_F(PopulationEventExecutionTest, ExecutesImportEventsWithEmptyWorkloads) {
  ImportationEvent importation(0, 0, 0, 0);
  importation.set_executable(true);
  importation.execute();
  EXPECT_FALSE(importation.is_executable());

  ImportationPeriodicallyEvent periodic(0, 30, 0, 0, 0);
  periodic.set_executable(true);
  periodic.execute();
  EXPECT_FALSE(periodic.is_executable());

  ImportationPeriodicallyRandomEvent random_periodic(0, 0, 0, 0.0);
  random_periodic.set_executable(true);
  random_periodic.execute();
  EXPECT_FALSE(random_periodic.is_executable());

  DistrictImportationDailyEvent district(1, 0.0, 0, {});
  district.set_executable(true);
  district.execute();
  EXPECT_FALSE(district.is_executable());
}

TEST_F(PopulationEventExecutionTest, ImportsParasiteIntoSusceptiblePopulation) {
  for (int i = 0; i < 100; ++i) {
    ASSERT_NE(Model::get_population()->give_1_birth(0), nullptr);
  }
  Model::get_mdc()->popsize_by_location_hoststate()[0][Person::SUSCEPTIBLE] = 100;
  Model::get_random()->set_seed(42);

  ImportationEvent importation(0, 0, 0, 1000);
  importation.set_executable(true);
  importation.execute();

  EXPECT_FALSE(importation.is_executable());
  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  bool any_infected = false;
  for (const auto& age_class : index->vPerson()[0][Person::ASYMPTOMATIC]) {
    for (auto* candidate : age_class) {
      any_infected = any_infected || !candidate->get_all_clonal_parasite_populations()->empty();
    }
  }
  EXPECT_TRUE(any_infected);
}

TEST_F(PopulationEventExecutionTest, ExecutesRandomImportationOnMonthBoundary) {
  Model::get_random()->set_seed(42);
  Model::get_scheduler()->set_current_time(30);  // January 31 in the generated fixture.

  ImportationPeriodicallyRandomEvent event(0, 30, 100, 10.0);
  event.set_executable(true);
  event.execute();

  EXPECT_FALSE(event.is_executable());
}

TEST_F(PopulationEventExecutionTest, ExecutesMutantEventsWithNoRequestedCases) {
  const std::vector<std::tuple<int, int, char>> alleles{{5, 86, 'Y'}};

  Introduce580YMutantEvent mutant_580y(0, 0, 0.0, alleles);
  IntroduceAmodiaquineMutantEvent amodiaquine(0, 0, 0.0, alleles);
  IntroduceLumefantrineMutantEvent lumefantrine(0, 0, 0.0, alleles);
  IntroduceTripleMutantToDPMEvent triple(0, 0, 0.0, alleles);
  IntroducePlas2CopyParasiteEvent plas2(0, 0, 0.0, alleles);

  for (Event* event : {static_cast<Event*>(&mutant_580y), static_cast<Event*>(&amodiaquine),
                       static_cast<Event*>(&lumefantrine), static_cast<Event*>(&triple),
                       static_cast<Event*>(&plas2)}) {
    event->set_executable(true);
    event->execute();
    EXPECT_FALSE(event->is_executable());
  }
}

TEST_F(PopulationEventExecutionTest, ExecutesMutantEventsForAnInfectedPerson) {
  auto* person = Model::get_population()->give_1_birth(0);
  ASSERT_NE(person, nullptr);
  person->set_host_state(Person::ASYMPTOMATIC);
  ASSERT_NE(person->add_new_parasite_to_blood(Model::get_genotype_db()->at(0)), nullptr);

  const std::vector<std::tuple<int, int, char>> alleles{{5, 86, 'Y'}};
  Introduce580YMutantEvent mutant_580y(0, 0, 100.0, alleles);
  IntroduceAmodiaquineMutantEvent amodiaquine(0, 0, 100.0, alleles);
  IntroduceLumefantrineMutantEvent lumefantrine(0, 0, 100.0, alleles);
  IntroduceTripleMutantToDPMEvent triple(0, 0, 100.0, alleles);
  IntroducePlas2CopyParasiteEvent plas2(0, 0, 100.0, alleles);

  for (Event* event : {static_cast<Event*>(&mutant_580y), static_cast<Event*>(&amodiaquine),
                       static_cast<Event*>(&lumefantrine), static_cast<Event*>(&triple),
                       static_cast<Event*>(&plas2)}) {
    event->set_executable(true);
    event->execute();
    EXPECT_FALSE(event->is_executable());
  }
}
