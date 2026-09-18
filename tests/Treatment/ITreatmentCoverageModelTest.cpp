#include <gtest/gtest.h>

#include <memory>

#include "Configuration/Config.h"
#include "Simulation/Model.h"
#include "Treatment/InflatedTCM.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Treatment/LinearTCM.h"
#include "Treatment/SteadyTCM.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ITreatmentCoverageModelTest : public ::testing::Test {
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

  Config* config() { return Model::get_config(); }
};

TEST_F(ITreatmentCoverageModelTest, BuildsSteadyModelAndSelectsAgeBand) {
  const auto node = YAML::Load(R"(
type: SteadyTCM
date: 2000/1/15
p_treatment_under_5_by_location: [0.2]
p_treatment_over_5_by_location: [0.4]
)");

  auto model = ITreatmentCoverageModel::build(node, config());

  ASSERT_NE(model, nullptr);
  EXPECT_EQ(model->type, "SteadyTCM");
  EXPECT_EQ(model->p_treatment_under_5.size(), config()->number_of_locations());
  EXPECT_NEAR(model->get_probability_to_be_treated(0, 5), 0.2, 1e-6);
  EXPECT_NEAR(model->get_probability_to_be_treated(0, 6), 0.4, 1e-6);
  EXPECT_THROW(model->get_probability_to_be_treated(
                   static_cast<core::LocationId>(config()->number_of_locations()), 5),
               std::invalid_argument);
}

TEST_F(ITreatmentCoverageModelTest, BuildsInflatedModelWithMonthlyRate) {
  const auto node = YAML::Load(R"(
type: InflatedTCM
date: 2000/2/1
annual_inflation_rate: 0.12
p_treatment_under_5_by_location: [0.1]
p_treatment_over_5_by_location: [0.3]
)");

  auto model = ITreatmentCoverageModel::build(node, config());
  auto* inflated = dynamic_cast<InflatedTCM*>(model.get());

  ASSERT_NE(inflated, nullptr);
  EXPECT_DOUBLE_EQ(inflated->monthly_inflation_rate, 0.01);
  EXPECT_EQ(inflated->starting_time, 31);
}

TEST_F(ITreatmentCoverageModelTest, BuildsLinearModelAndCopiesTransitionEndpoints) {
  const auto node = YAML::Load(R"(
type: LinearTCM
from_date: 2000/1/1
to_date: 2000/4/1
p_treatment_under_5_by_location_from: [0.1]
p_treatment_under_5_by_location_to: [0.2]
p_treatment_over_5_by_location_from: [0.3]
p_treatment_over_5_by_location_to: [0.4]
)");

  auto model = ITreatmentCoverageModel::build(node, config());
  auto* linear = dynamic_cast<LinearTCM*>(model.get());

  ASSERT_NE(linear, nullptr);
  EXPECT_EQ(linear->starting_time, 0);
  EXPECT_EQ(linear->end_time, 91);
  EXPECT_NEAR(linear->p_treatment_under_5.front(), 0.1, 1e-6);
  EXPECT_NEAR(linear->p_treatment_under_5_to.front(), 0.2, 1e-6);
  EXPECT_NEAR(linear->p_treatment_over_5_to.front(), 0.4, 1e-6);
}

TEST_F(ITreatmentCoverageModelTest, UnknownTypeReturnsNullAndEmptyInputIsIgnored) {
  std::vector<double> values{0.7};
  ITreatmentCoverageModel::read_p_treatment(
      YAML::Node(YAML::NodeType::Undefined), values, config()->number_of_locations());
  EXPECT_EQ(values.size(), 1);

  const auto node = YAML::Load("type: UnsupportedTCM");
  EXPECT_EQ(ITreatmentCoverageModel::build(node, config()), nullptr);
}
