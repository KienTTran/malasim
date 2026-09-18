#include <gtest/gtest.h>

#include "Events/Population/ImportationPeriodicallyEvent.h"
#include "Events/Population/ImportationPeriodicallyRandomEvent.h"
#include "Events/ProgressToClinicalEvent.h"
#include "MDC/ModelDataCollector.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "fixtures/TestFileGenerators.h"

class ImportationEventCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override {
  Model::get_scheduler()->set_current_time(0);
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  void add_susceptible_people(const int count) {
    for (int i = 0; i < count; ++i) {
      ASSERT_NE(Model::get_population()->give_1_birth(0), nullptr);
    }
    Model::get_mdc()->popsize_by_location_hoststate()[0][Person::SUSCEPTIBLE] = count;
  }
};

TEST_F(ImportationEventCoverageTest, ClinicalRandomImportSchedulesProgression) {
  add_susceptible_people(100);
  Model::get_random()->set_seed(42);

  ImportationPeriodicallyRandomEvent event(0, 0, 100, 10.0);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());

  bool scheduled_progression = false;
  auto *index =
      Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  for (const auto &location : index->vPerson()) {
    for (const auto &state : location) {
      for (const auto &age_class : state) {
        for (auto *person : age_class) {
          scheduled_progression = scheduled_progression ||
                                  person->has_event<ProgressToClinicalEvent>();
        }
      }
    }
  }
  EXPECT_TRUE(scheduled_progression);
}

TEST_F(ImportationEventCoverageTest, PeriodicImportSupportsRandomGenotypeModes) {
  add_susceptible_people(100);
  Model::get_random()->set_seed(42);

  ImportationPeriodicallyEvent random_even(0, 1, -1, 20, 0);
  random_even.set_executable(true);
  ASSERT_NO_THROW(random_even.execute());

  ImportationPeriodicallyEvent random_any(0, 1, -2, 20, 0);
  random_any.set_executable(true);
  ASSERT_NO_THROW(random_any.execute());
}

TEST_F(ImportationEventCoverageTest, RandomImportStopsWhenNoSusceptiblePeopleRemain) {
  Model::get_random()->set_seed(7);
  const auto starting_date = Model::get_config()->get_simulation_timeframe().get_starting_date();
  const auto month_end = date::year_month_day{date::sys_days{starting_date} + date::days{30}};
  const auto next_day = date::year_month_day{date::sys_days{starting_date} + date::days{31}};
  Model::get_scheduler()->initialize(month_end, next_day);

  ImportationPeriodicallyRandomEvent event(0, 0, 1000, 1.0);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());
  EXPECT_FALSE(event.is_executable());
}
