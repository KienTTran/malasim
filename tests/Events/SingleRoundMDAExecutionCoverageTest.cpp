#include <gtest/gtest.h>

#include "Events/Population/SingleRoundMDAEvent.h"
#include "Events/ReceiveMDATherapyEvent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "fixtures/TestFileGenerators.h"

class SingleRoundMDAExecutionCoverageTest : public ::testing::Test {
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

TEST_F(SingleRoundMDAExecutionCoverageTest, SchedulesTherapyForTargetedPopulation) {
  for (int i = 0; i < 1; ++i) {
    auto* person = Model::get_population()->give_1_birth(0);
    ASSERT_NE(person, nullptr);
    person->set_prob_present_at_mda_by_age({1.0});
  }

  SingleRoundMDAEvent event(std::vector<double>(Model::get_config()->number_of_locations(), 1.0));
  event.set_days_to_complete(3);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());

  int scheduled_count = 0;
  for (const auto &person : Model::get_population()
                                ->get_person_index<PersonIndexByLocationStateAgeClass>()
                                ->vPerson()[0][Person::SUSCEPTIBLE][0]) {
    scheduled_count += person->has_event<ReceiveMDATherapyEvent>() ? 1 : 0;
  }
  EXPECT_GT(scheduled_count, 0);
}
