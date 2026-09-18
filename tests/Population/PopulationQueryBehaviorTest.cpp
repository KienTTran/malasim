#include <gtest/gtest.h>

#include "Population/Population.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Index/PersonIndexByLocationMovingLevel.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "fixtures/TestFileGenerators.h"

class PopulationQueryBehaviorTest : public ::testing::Test {
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

TEST_F(PopulationQueryBehaviorTest, QueriesLocationStateAgeAndResidenceCounts) {
  auto* population = Model::get_population();
  ASSERT_GT(population->size(), 0u);

  EXPECT_EQ(population->size(-1), population->size());
  EXPECT_NO_THROW(population->size(0, core::K_INVALID_AGE_CLASS));
  EXPECT_NO_THROW(population->size(0, Person::SUSCEPTIBLE, 0));
  EXPECT_NO_THROW(population->size_residents_only(0));
  EXPECT_EQ(population->size_residents_only(-1), population->size());
  EXPECT_NO_THROW(population->has_0_case());

  auto *state_age_index =
      population->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto *moving_level_index =
      population->get_person_index<PersonIndexByLocationMovingLevel>();
  ASSERT_NE(state_age_index, nullptr);
  ASSERT_NE(moving_level_index, nullptr);
  state_age_index->update();
  moving_level_index->update();
  EXPECT_EQ(state_age_index->size(), 0U);
  EXPECT_EQ(moving_level_index->size(), 0U);
}
