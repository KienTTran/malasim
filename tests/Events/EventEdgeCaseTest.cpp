#include <gtest/gtest.h>

#include "Events/Population/ChangeTreatmentCoverageEvent.h"
#include "Events/Population/SingleRoundMDAEvent.h"
#include "Events/RaptEvent.h"
#include "Events/ReceiveMDATherapyEvent.h"
#include "Events/ReportTreatmentFailureDeathEvent.h"
#include "Simulation/Model.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class EventEdgeCaseTest : public ::testing::Test {
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

TEST_F(EventEdgeCaseTest, PersonEventsRejectMissingPerson) {
  RaptEvent rapt(nullptr);
  rapt.set_executable(true);
  EXPECT_NO_THROW(rapt.execute());

  ReceiveMDATherapyEvent mda(nullptr);
  mda.set_executable(true);
  EXPECT_NO_THROW(mda.execute());

  ReportTreatmentFailureDeathEvent failure(nullptr);
  failure.set_executable(true);
  EXPECT_NO_THROW(failure.execute());
}

TEST_F(EventEdgeCaseTest, ChangeTreatmentCoverageEventInstallsBuiltModel) {
  auto node = YAML::Load(R"(
type: SteadyTCM
date: 2000/1/1
p_treatment_under_5_by_location: [0.1]
p_treatment_over_5_by_location: [0.2]
)");
  auto event = std::make_unique<ChangeTreatmentCoverageEvent>(
      ITreatmentCoverageModel::build(node, Model::get_config()));
  event->set_executable(true);
  event->execute();

  ASSERT_NE(Model::get_treatment_coverage(), nullptr);
  EXPECT_NEAR(Model::get_treatment_coverage()->get_probability_to_be_treated(0, 5), 0.1, 1e-6);
  EXPECT_FALSE(event->is_executable());
}

TEST_F(EventEdgeCaseTest, EmptySingleRoundMdaExecutesWithoutPersons) {
  SingleRoundMDAEvent event(std::vector<double>(Model::get_config()->number_of_locations(), 0.0));
  event.set_executable(true);
  EXPECT_NO_THROW(event.execute());
}
