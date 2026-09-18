#include <gtest/gtest.h>

#include "Events/Population/IntroduceParasitesPeriodicallyEventV2.h"
#include "Events/Population/PopulationEventBuilder.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventV2BuilderTest : public ::testing::Test {
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

TEST_F(PopulationEventV2BuilderTest, ExpandsNegativeLocationSentinelToEveryLocation) {
  const auto events = PopulationEventBuilder::build_introduce_parasites_periodically_events_v2(
      YAML::Load(R"(
- location: -1
  parasite_info:
    - duration: 3
      number_of_cases: 2
      start_date: 2024/01/02
      end_date: 2024/01/05
)"),
      Model::get_config());

  ASSERT_EQ(events.size(), Model::get_config()->number_of_locations());
  for (std::size_t location = 0; location < events.size(); ++location) {
    const auto* event = dynamic_cast<const IntroduceParasitesPeriodicallyEventV2*>(events[location].get());
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->location(), static_cast<int>(location));
    EXPECT_EQ(event->duration(), 3);
    EXPECT_EQ(event->number_of_cases(), 2);
    EXPECT_EQ(event->start_day,
              (date::sys_days{date::year{2024} / 1 / 2}
               - date::sys_days{Model::get_config()->get_simulation_timeframe().get_starting_date()})
                  .count());
    EXPECT_EQ(event->end_day,
              (date::sys_days{date::year{2024} / 1 / 5}
               - date::sys_days{Model::get_config()->get_simulation_timeframe().get_starting_date()})
                  .count());
  }
}
