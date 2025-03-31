/*
 * EnvironmentEventBuilder.cpp
 *
 * Implement the environmental event builders.
 */
#include "EnvironmentEventBuilder.h"

#include <vector>

#include "Configuration/Config.h"
#include "UpdateEcozoneEvent.hxx"
#include "Utils/Helpers/StringHelpers.h"
#include "yaml-cpp/yaml.h"

std::vector<Event*> EnvironmentEventBuilder::build(const YAML::Node &node) {
  std::vector<Event*> events;
  const auto name = node["name"].as<std::string>();

  if (name == UpdateEcozoneEvent::EventName) {
    events = build_update_ecozone_event(node["info"], Model::get_config());
  }

  return events;
}

std::vector<Event*> EnvironmentEventBuilder::build_update_ecozone_event(
    const YAML::Node &node, Config* config) {
  try {
    std::vector<Event*> events;
    for (std::size_t ndx = 0; ndx < node.size(); ndx++) {
      // Load the values
      auto start_date = node[ndx]["day"].as<date::year_month_day>();
      const auto time =
          (date::sys_days{start_date} - date::sys_days{config->get_simulation_timeframe().get_starting_date()})
              .count();
      auto from = node[ndx]["from"].as<int>();
      auto to = node[ndx]["to"].as<int>();

      // Verify inputs
      if (from < 0) {
        spdlog::error("Invalid from value supplied from for {} "
                      "value supplied cannot be less than zero",
                      UpdateEcozoneEvent::EventName);
        throw std::invalid_argument("From value cannot be less than zero");
      }
      if (to < 0) {
        spdlog::error("Invalid from value supplied to for {} "
                      "value supplied cannot be less than zero",
                      UpdateEcozoneEvent::EventName);
        throw std::invalid_argument("To value cannot be less than zero");
      }

      // Log and add the event to the queue
      auto* event = new UpdateEcozoneEvent(from, to, time);
      spdlog::debug("Adding event {} start date: {} from: {} to: {}",
                    event->name(), StringHelpers::date_as_string(start_date), from, to);
      events.push_back(event);
    }
    return events;

  } catch (YAML::BadConversion &error) {
    spdlog::error("Unrecoverable error parsing YAML value in "
                  "UpdateEcozoneEvent node: {}", error.msg);
    exit(1);
  }
}
