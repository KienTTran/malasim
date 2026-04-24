#include "SwitchImmuneComponentEvent.h"

#include <cassert>
#include <memory>

#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/ImmuneSystem/NonInfantImmuneComponent.h"
#include "Population/Person/Person.h"

// OBJECTPOOL_IMPL(SwitchImmuneComponentEvent)

SwitchImmuneComponentEvent::SwitchImmuneComponentEvent(Person* person) : PersonEvent(person) {
  if (person == nullptr) {
    spdlog::error("SwitchImmuneComponentEvent::SwitchImmuneComponentEvent, person is nullptr");
    throw std::invalid_argument(
        "SwitchImmuneComponentEvent::SwitchImmuneComponentEvent, person is nullptr");
  }
}

SwitchImmuneComponentEvent::~SwitchImmuneComponentEvent() = default;

// void SwitchImmuneComponentEvent::do_execute() {
//   auto* person = get_person();
//   if (person == nullptr) {
//     spdlog::error("SwitchImmuneComponentEvent::do_execute, person is nullptr");
//     throw std::invalid_argument("SwitchImmuneComponentEvent::do_execute, person is nullptr");
//   }
//   person->get_immune_system()->set_immune_component(std::make_unique<NonInfantImmuneComponent>());
// }

void SwitchImmuneComponentEvent::do_execute() {
    auto* person = get_person();
    if (person == nullptr) {
        throw std::invalid_argument("SwitchImmuneComponentEvent::do_execute, person is nullptr");
    }

    // Capture current immune value BEFORE replacing the component
    const double current_immune_value = person->get_immune_system()->get_current_value();

    auto new_component = std::make_unique<NonInfantImmuneComponent>();
    // Transfer the current immune value so the transition is continuous
    // Without this, latest_value_ defaults to 0.0 and clinical_progression_probability
    // immediately jumps to max for the rest of year 0
    new_component->set_latest_value(current_immune_value);

    person->get_immune_system()->set_immune_component(std::move(new_component));
}

