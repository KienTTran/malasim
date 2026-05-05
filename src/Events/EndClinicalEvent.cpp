/*
 * File:   EndClinicalEvent.cpp
 * Author: Merlin
 *
 * Created on July 31, 2013, 12:27 PM
 */

#include "EndClinicalEvent.h"

#include "Core/Scheduler/Scheduler.h"
#include "Events/TestTreatmentFailureEvent.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"

// OBJECTPOOL_IMPL(EndClinicalEvent)

void EndClinicalEvent::do_execute() {
  auto* person = get_person();

  if (person == nullptr) { throw std::runtime_error("Person is nullptr"); }

  // Fix 1: Dead persons must never be processed here.
  // set_host_state(DEAD) clears all parasite populations, so the size()==0 branch below
  // would call change_state_when_no_parasite_in_blood() which transitions DEAD→SUSCEPTIBLE,
  // resurrecting the person and letting them accumulate extra clinical episodes.
  if (person->get_host_state() == Person::DEAD) { return; }

  if (person->get_all_clonal_parasite_populations()->size() == 0) {
    person->change_state_when_no_parasite_in_blood();

  } else {
    // still have parasite in blood
    person->get_immune_system()->set_increase(true);

    // Fix 2 (defense-in-depth): only move the person to ASYMPTOMATIC when they are not
    // currently CLINICAL from a *different* parasite.  If a new clinical episode started
    // (different parasite) before this stale EndClinicalEvent fired, overriding the state
    // here would prematurely terminate that active episode and open a second recrudescence
    // pathway — causing extra clinical episode counts.
    if (person->get_host_state() != Person::CLINICAL) {
      person->set_host_state(Person::ASYMPTOMATIC);
    }

    if (person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_)) {
      // Fix 3: Only evaluate recrudescence when the person was actually TREATED.
      //
      // For untreated persons the parasite remains under clinical_update_function, which
      // resets density to log_parasite_density_asymptomatic (~3) every day.  This means:
      //   (a) the density threshold > 2 is ALWAYS met, and
      //   (b) there is no TestTreatmentFailureEvent to cancel, so no TF is ever recorded.
      // The result is a self-reinforcing spurious recurrence chain:
      //   episode → EndClinicalEvent → recrudescence → episode → EndClinicalEvent → ...
      //
      // "Was treated" is detected by the presence of a still-executable
      // TestTreatmentFailureEvent for this exact parasite.
      bool was_treated = false;
      for (const auto &[time, event] : person->get_events()) {
        auto* tf_event = dynamic_cast<TestTreatmentFailureEvent*>(event.get());
        if (tf_event != nullptr && tf_event->is_executable()
            && tf_event->clinical_caused_parasite() == clinical_caused_parasite_) {
          was_treated = true;
          break;
        }
      }

      if (was_treated) {
        // Treatment was given — evaluate whether surviving parasite causes symptomatic
        // recrudescence (legitimate treatment-failure pathway).
        person->determine_symptomatic_recrudescence(clinical_caused_parasite_);
      } else {
        // No treatment — hand the parasite to natural immunity so it clears without
        // generating a recurrence.
        clinical_caused_parasite_->set_update_function(
            Model::immunity_clearance_update_function());
      }
    }
  }
}
