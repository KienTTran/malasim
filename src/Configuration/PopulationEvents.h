#pragma once
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>

#include "Events/Environment/EnvironmentEventBuilder.h"
#include "Events/Population/PopulationEventBuilder.h"
#include "date/date.h"
#include "Utils/YamlFile.h"

class PopulationEvents {
public:
  // Inner class: EventInfo
  class EventInfo {
  public:
    // Getters and Setters
    [[nodiscard]] const date::year_month_day &get_date() const { return date_; }
    void set_date(const date::year_month_day &value) { date_ = value; }

  private:
    date::year_month_day date_ =
        date::year_month_day(date::year(2000), date::month(1), date::day(1));
  };

  // Inner class: PopulationEvent
  class PopulationEvent {
  public:
    // Getters and Setters
    [[nodiscard]] const std::string &get_name() const { return name_; }
    void set_name(const std::string &value) { name_ = value; }

    [[nodiscard]] const std::vector<EventInfo> &get_info() const { return info_; }
    void set_info(const std::vector<EventInfo> &value) { info_ = value; }

  private:
    std::string name_;
    std::vector<EventInfo> info_;
  };

  // Getters and Setters for PopulationEvents
  [[nodiscard]] const std::vector<PopulationEvent> &get_events_raw() const { return events_raw_; }
  void set_events_raw(const std::vector<PopulationEvent> &value) { events_raw_ = value; }

  [[nodiscard]] const std::vector<std::unique_ptr<WorldEvent>> &get_events() { return events_; }
  void set_events(std::vector<std::unique_ptr<WorldEvent>> &&value) { events_ = std::move(value); }

  // Method to transfer ownership of the events vector
  [[nodiscard]] std::vector<std::unique_ptr<WorldEvent>> release_events() {
    return std::move(events_);
  }

private:
  std::vector<PopulationEvent> events_raw_;
  std::vector<std::unique_ptr<WorldEvent>> events_;
};

// Conversion functions inside namespace YAML
namespace YAML {


template <>
struct convert<PopulationEvents::EventInfo> {
  static Node encode(const PopulationEvents::EventInfo &rhs) {
    Node node;
    node["date"] = rhs.get_date();
    return node;
  }

  static bool decode(const Node &node, PopulationEvents::EventInfo &rhs) {
    if (node["date"]) { rhs.set_date(node["date"].as<date::year_month_day>()); }
    return true;
  }
};

template <>
struct convert<PopulationEvents::PopulationEvent> {
  static Node encode(const PopulationEvents::PopulationEvent &rhs) {
    Node node;
    node["name"] = rhs.get_name();
    node["info"] = rhs.get_info();
    return node;
  }

  static bool decode(const Node &node, PopulationEvents::PopulationEvent &rhs) {
    if (!node["name"]) {
      throw std::runtime_error("Missing 'event_name' field in PopulationEvent.");
    }
    if (!node["info"]) { throw std::runtime_error("Missing 'info' field in PopulationEvent."); }

    rhs.set_name(node["name"].as<std::string>());
    rhs.set_info(node["info"].as<std::vector<PopulationEvents::EventInfo>>());
    return true;
  }
};

template <>
struct convert<PopulationEvents> {
  static Node encode(const PopulationEvents &rhs) {
    Node node;
    for (const auto &event : rhs.get_events_raw()) {
      node["population_events"].push_back(event);  // Push each event into the list
    }
    return node;
  }
  static bool decode(const Node &node, PopulationEvents &rhs) {
    spdlog::info("Decoding PopulationEvents");
    std::vector<PopulationEvents::PopulationEvent> events_raw;
    std::vector<std::unique_ptr<WorldEvent>> events_unique;
    for (const auto &event_node : node) {
      PopulationEvents::PopulationEvent event;
      convert<PopulationEvents::PopulationEvent>::decode(event_node, event);
      events_raw.push_back(event);
      spdlog::info("Parsing event: {}", event_node["name"].as<std::string>());

      auto built_events_pop = PopulationEventBuilder::build(event_node);
      if (!built_events_pop.empty()) {
        for (auto &event_ptr : built_events_pop) { events_unique.push_back(std::move(event_ptr)); }
        continue;
      }

      auto built_events_env = EnvironmentEventBuilder::build(event_node);
      if (!built_events_env.empty()) {
        for (auto &event_ptr : built_events_env) { events_unique.push_back(std::move(event_ptr)); }
      }
    }
    rhs.set_events_raw(events_raw);
    rhs.set_events(std::move(events_unique));

    return true;
  }
};

}  // namespace YAML
