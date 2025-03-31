/*
 * UpdateEcozoneEvent.hxx
 *
 * Update the ecozone from the previous type to the new type.
 */
#ifndef UPDATEECOZONEEVENT_H
#define UPDATEECOZONEEVENT_H

#include "Configuration/Config.h"
#include "Configuration/SeasonalitySettings.h"
#include "Events/Event.h"
#include "Simulation/Model.h"
#include <spdlog/spdlog.h>

class UpdateEcozoneEvent : public Event {
public:
  //disallow copy, assign and move
  UpdateEcozoneEvent(const UpdateEcozoneEvent&) = delete;
  void operator=(const UpdateEcozoneEvent&) = delete;
  UpdateEcozoneEvent(UpdateEcozoneEvent&&) = delete;
  UpdateEcozoneEvent& operator=(UpdateEcozoneEvent&&) = delete;

private:
  int from_;
  int to_;

  // Execute the import event
  void execute() override {
    // Scan all the locations, if they match the old values, then replace them
    // with the new
    spdlog::info("Updating ecozone from {} to {}", from_, to_);

    // Defer to the object for the actual update
    auto *seasons = Model::get_config()->get_seasonality_settings().get_seasonal_equation();
    // if (seasons == nullptr) {
    //   throw std::runtime_error(
    //       "Configuration called for seasonality to be updated with a mode that "
    //       "does not support it.");
    // }
    seasons->update_seasonality(from_, to_);
  }

public:
  inline static const std::string EventName = "update_ecozone_event";

  UpdateEcozoneEvent(const int from, const int to, const int start) : from_(from), to_(to) {
    time = start;
  }

  ~UpdateEcozoneEvent() override = default;

  std::string name() override { return EventName; }
};

#endif
