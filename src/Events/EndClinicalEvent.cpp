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

  if (person == nullptr) {
    throw std::runtime_error("Person is nullptr");
  }

  // Dead persons must never be processed here.
  //
  // set_host_state(DEAD) clears all parasite populations. Without this guard,
  // the size()==0 branch below would call change_state_when_no_parasite_in_blood(),
  // which can incorrectly transition DEAD -> SUSCEPTIBLE.
  if (person->get_host_state() == Person::DEAD) {
    person->set_current_clinical_caused_parasite(nullptr);
    return;
  }

  // No parasite remains in blood.
  //
  // There is no recrudescence decision to make when the clinical-causing parasite
  // has already been cleared. Move the person according to the normal no-parasite
  // rule and clear the active clinical-owner pointer.
  if (person->get_all_clonal_parasite_populations()->size() == 0) {
    person->change_state_when_no_parasite_in_blood();
    person->set_current_clinical_caused_parasite(nullptr);
    return;
  }

  // Parasites are still present, so immune response should continue increasing.
  person->get_immune_system()->set_increase(true);

  // Only the EndClinicalEvent belonging to the currently active clinical-causing
  // parasite is allowed to end the CLINICAL state.
  //
  // This protects against stale EndClinicalEvents. For example:
  //
  //   episode A starts -> EndClinicalEvent A scheduled
  //   episode B starts before A ends
  //   stale EndClinicalEvent A fires
  //
  // In that case, A must not move the person CLINICAL -> ASYMPTOMATIC if B is now
  // the active clinical episode.
  const bool owns_current_clinical_episode =
      person->get_host_state() == Person::CLINICAL &&
      person->get_current_clinical_caused_parasite() == clinical_caused_parasite_;

  if (owns_current_clinical_episode) {
    person->set_host_state(Person::ASYMPTOMATIC);
    person->set_current_clinical_caused_parasite(nullptr);
  }

  // If the clinical-causing parasite is no longer present, there is nothing to
  // evaluate for recrudescence.
  if (!person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_)) {
    return;
  }

  // Recrudescence should only be evaluated for treated episodes.
  //
  // In this model, symptomatic recrudescence represents a treatment-failure
  // pathway:
  //
  //   clinical episode -> treatment -> parasite survives -> symptoms return
  //
  // Untreated episodes should not enter determine_symptomatic_recrudescence().
  // Otherwise untreated parasites can repeatedly generate:
  //
  //   episode -> EndClinicalEvent -> recrudescence -> episode -> ...
  //
  // without any treatment-failure event.
  bool was_treated = false;

  for (const auto& [time, event] : person->get_events()) {
    auto* tf_event = dynamic_cast<TestTreatmentFailureEvent*>(event.get());

    if (tf_event != nullptr &&
        tf_event->is_executable() &&
        tf_event->clinical_caused_parasite() == clinical_caused_parasite_) {
      was_treated = true;
      break;
    }
  }

  if (was_treated && !is_recurrence_) {
    // First treated clinical episode:
    // allow one symptomatic recrudescence decision.
    person->determine_symptomatic_recrudescence(clinical_caused_parasite_);
    return;
  }

  // Either:
  //   1. no treatment was given, or
  //   2. this episode was already a recrudescence episode.
  //
  // In both cases, do not allow another recrudescence loop.
  if (person->has_effective_drug_in_blood()) {
    clinical_caused_parasite_->set_update_function(
        Model::having_drug_update_function());
  } else {
    clinical_caused_parasite_->set_update_function(
        Model::immunity_clearance_update_function());
  }
}