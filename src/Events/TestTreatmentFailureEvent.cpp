#include "TestTreatmentFailureEvent.h"

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Simulation/Model.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person/Person.h"
#include "Population/Person/FollowupEpisodeTypes.h"

//OBJECTPOOL_IMPL(TestTreatmentFailureEvent)


void TestTreatmentFailureEvent::do_execute() {
  auto* person = get_person();
  if (person == nullptr) {
    throw std::runtime_error("Person is nullptr");
  }

  // If the parasite is still present at a detectable level, then it's a
  // treatment failure
  if (person->get_all_clonal_parasite_populations()->contain(
          clinical_caused_parasite())
      && clinical_caused_parasite_->last_update_log10_parasite_density()
             > Model::get_config()->get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_detectable()) {
    Model::get_mdc()->record_1_treatment_failure_by_therapy(
        person->get_location(), person->get_age_class(), therapy_id_);
    // Flush pending 28-day follow-up events with failure outcome if this is
    // the treatment that opened the follow-up window.
    if (person->has_active_first_treatment_followup_window()
        && clinical_caused_parasite_ == person->first_treatment_followup_parasite()
        && therapy_id_ == person->first_treatment_followup_therapy_id()) {
      person->flush_pending_followup_events_with_first_treatment_outcome_and_reset(
          FirstTreatmentOutcome::Failure);
    }
  } else {
    Model::get_mdc()->record_1_treatment_success_by_therapy(
        person->get_location(), person->get_age_class(), therapy_id_);
    // Flush pending 28-day follow-up events with success outcome if this is
    // the treatment that opened the follow-up window.
    if (person->has_active_first_treatment_followup_window()
        && clinical_caused_parasite_ == person->first_treatment_followup_parasite()
        && therapy_id_ == person->first_treatment_followup_therapy_id()) {
      person->flush_pending_followup_events_with_first_treatment_outcome_and_reset(
          FirstTreatmentOutcome::Success);
    }
  }
}
