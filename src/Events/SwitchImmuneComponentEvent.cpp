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

void SwitchImmuneComponentEvent::do_execute() {
  auto* person = get_person();
  if (person == nullptr) {
    spdlog::error("SwitchImmuneComponentEvent::do_execute, person is nullptr");
    throw std::invalid_argument("SwitchImmuneComponentEvent::do_execute, person is nullptr");
  }

  auto* immune_system = person->get_immune_system();
  if (immune_system == nullptr) {
    spdlog::error("SwitchImmuneComponentEvent::do_execute, immune_system is nullptr");
    throw std::invalid_argument("SwitchImmuneComponentEvent::do_execute, immune_system is nullptr");
  }

  // Preserve immunity at switch time to avoid resetting to 0
  const double preserved_I = immune_system->get_current_value();  // or get_latest_immune_value()

  immune_system->set_immune_component(std::make_unique<NonInfantImmuneComponent>());

  // Restore the preserved immunity value into the new component
  if (immune_system->immune_component() == nullptr) {
    spdlog::error("SwitchImmuneComponentEvent::do_execute, immune_component is nullptr after set");
    throw std::runtime_error(
        "SwitchImmuneComponentEvent::do_execute, immune_component is nullptr after set");
  }
  immune_system->immune_component()->set_latest_value(preserved_I);
}
