#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "Configuration/PopulationEvents.h"  // Assuming this is the correct include path

class PopulationEventsTest : public ::testing::Test {
protected:
    PopulationEvents population_events;

    void SetUp() override {
        // Set up default PopulationEvents

        // Setting up EventInfo
        PopulationEvents::EventInfo event_info;
        event_info.set_date(date::year_month_day{date::year(2024), date::month(10), date::day(1)});
        event_info.set_location_id(1);

        // Setting up PopulationEvent
        PopulationEvents::PopulationEvent population_event;
        population_event.set_name("introduce_parasites");
        population_event.set_info({event_info});

        // Adding event to PopulationEvents
        population_events.set_events({population_event});
    }
};

// Test encoding functionality
TEST_F(PopulationEventsTest, EncodePopulationEvents) {
    YAML::Node node = YAML::convert<PopulationEvents>::encode(population_events);

    ASSERT_EQ(node["population_events"][0]["name"].as<std::string>(), "introduce_parasites");

    const auto& event_info_node = node["population_events"][0]["info"][0];
    EXPECT_EQ(event_info_node["date"].as<std::string>(), "2024/10/01");
    EXPECT_EQ(event_info_node["location_id"].as<int>(), 1);
}

// Test decoding functionality
TEST_F(PopulationEventsTest, DecodePopulationEvents) {
    YAML::Node node;
    node["population_events"][0]["name"] = "introduce_parasites";
    node["population_events"][0]["info"][0]["date"] = "2024/10/01";
    node["population_events"][0]["info"][0]["location_id"] = 1;

    PopulationEvents decoded_population_events;
    ASSERT_NO_THROW(YAML::convert<PopulationEvents>::decode(node, decoded_population_events));

    const auto& events = decoded_population_events.get_events();
    ASSERT_EQ(events.size(), 1);
    const auto& event_info = events[0].get_info()[0];

    EXPECT_EQ(event_info.get_date().year(), date::year(2024));
    EXPECT_EQ(event_info.get_location_id(), 1);
}

// Test missing fields during decoding
TEST_F(PopulationEventsTest, DecodePopulationEventsMissingField) {
    YAML::Node node;
    node["population_events"][0]["name"] = "introduce_parasites";  // Missing "info" field

    PopulationEvents decoded_population_events;
    EXPECT_THROW(YAML::convert<PopulationEvents>::decode(node, decoded_population_events), std::runtime_error);
}
