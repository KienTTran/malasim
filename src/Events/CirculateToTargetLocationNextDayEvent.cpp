/*
 * CirculateToTargetLocationNextDayEvent.cpp
 *
 * Implement the event to move the individual to the next location.
 */
#include "CirculateToTargetLocationNextDayEvent.h"

#include "Configuration/Config.h"
#include "Utils/Random.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Population/Person/Person.h"
#include "ReturnToResidenceEvent.h"
#include "Population/Population.h"

//OBJECTPOOL_IMPL(CirculateToTargetLocationNextDayEvent)

void CirculateToTargetLocationNextDayEvent::do_execute() {
  // Get the person and perform the movement
  auto* person = get_person();
  if (person == nullptr) {
    throw std::runtime_error("Person is nullptr");
  }
  auto source_location = person->get_location();
  person->set_location(target_location_);

  // Notify the population of the change
  Model::get_population()->notify_movement(source_location, target_location_);

  // Did we randomly arrive back at the residence location?
  if (target_location_ == person->get_residence_location()) {
    // Cancel any outstanding return trips and return
    person->cancel_all_return_to_residence_events();
    return;
  }

  // If the person already ahs a return trip scheduled then we can return
  if (person->has_return_to_residence_event()) { return; }

  // Since a return is not scheduled, we need to create a return event based
  // upon a gamma distribution
  auto length_of_trip = 0;
  while (length_of_trip < 1) {
    length_of_trip = static_cast<int>(std::round(Model::get_random()->random_gamma(
        Model::get_config()->get_movement_settings().get_length_of_stay_theta(),
        Model::get_config()->get_movement_settings().get_length_of_stay_k())));
  }
  
  person->schedule_return_to_residence_event(length_of_trip);
}
