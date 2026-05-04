/*
 * File:   ProgressToClinicalEvent.cpp
 * Author: Merlin
 *
 * Created on July 30, 2013, 2:36 PM
 */

#include "ProgressToClinicalEvent.h"

#include <algorithm>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Events/ReportTreatmentFailureDeathEvent.h"
#include "MDC/ModelDataCollector.h"
#include "Population/ClinicalUpdateFunction.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person/Person.h"
#include "Population/Population.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Simulation/Model.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Treatment/Strategies/IStrategy.h"
#include "Treatment/Strategies/NestedMFTStrategy.h"
#include "Events/TestTreatmentFailureEvent.h"
#include "Utils/Random.h"

// OBJECTPOOL_IMPL(ProgressToClinicalEvent)
namespace {

  bool record_original_treatment_failure_if_recrudescence_before_tf_test(
      Person* person,
      ClonalParasitePopulation* clinical_parasite
  ) {
    if (person == nullptr || clinical_parasite == nullptr) {
      return false;
    }

    for (auto& [time, event] : person->get_events()) {
      auto* tf_event = dynamic_cast<TestTreatmentFailureEvent*>(event.get());

      if (tf_event == nullptr) {
        continue;
      }

      if (!event->is_executable()) {
        continue;
      }

      if (tf_event->clinical_caused_parasite() != clinical_parasite) {
        continue;
      }

      event->set_executable(false);

      // This records the outcome of the ORIGINAL treatment.
      Model::get_mdc()->record_1_tf(person->get_location(), true);
      Model::get_mdc()->record_1_treatment_failure_by_therapy(
          person->get_location(),
          person->get_age_class(),
          tf_event->therapy_id()
      );

      return true;
    }

    return false;
  }

}  // namespace

bool ProgressToClinicalEvent::should_receive_treatment(Person* person) {
  const double base_p = Model::get_treatment_coverage()->get_probability_to_be_treated(person->get_location(),
                                                                                       person->get_age());
  const auto &ep = Model::get_config()->get_epidemiological_parameters();
  const double modifier = ep.get_age_based_probability_of_seeking_treatment().evaluate_for_age(person->get_age());
  const double effective_p = std::clamp(base_p * modifier, 0.0, 1.0);
  return Model::get_random()->random_flat(0.0, 1.0) <= effective_p;
}

void ProgressToClinicalEvent::handle_no_treatment(Person* person) {
  // did not receive treatment
  Model::get_mdc()->record_1_tf(person->get_location(), false);
  Model::get_mdc()->record_1_non_treated_case(person->get_location(), person->get_age(),
                                              person->get_age_class());

  if (person->will_progress_to_death_when_receive_no_treatment()) {
    person->cancel_all_events_except(nullptr);
    person->set_host_state(Person::DEAD);
    Model::get_mdc()->record_1_malaria_death(person->get_location(), person->get_age(), false);
    return;
  }
}
std::pair<Therapy*, bool> ProgressToClinicalEvent::determine_therapy(
    Person* person,
    bool is_recurrence
) {
  auto* strategy = dynamic_cast<NestedMFTStrategy*>(Model::get_treatment_strategy());

  if (strategy != nullptr) {
    const auto probability = Model::get_random()->random_flat(0.0, 1.0);

    double sum = 0;
    std::size_t s_id = 0;

    for (std::size_t i = 0; i < strategy->distribution.size(); i++) {
      sum += strategy->distribution[i];
      if (probability <= sum) {
        s_id = i;
        break;
      }
    }

    const bool is_public_sector = (s_id == 0);

    if (is_recurrence) {
      const auto recurrent_therapy_id =
          Model::get_config()->get_therapy_parameters().get_recurrent_therapy_id();

      if (recurrent_therapy_id != -1) {
        return {Model::get_therapy_db()[recurrent_therapy_id].get(), false};
      }
    }

    return {strategy->strategy_list[s_id]->get_therapy(person), is_public_sector};
  }

  if (is_recurrence) {
    const auto recurrent_therapy_id =
        Model::get_config()->get_therapy_parameters().get_recurrent_therapy_id();

    if (recurrent_therapy_id != -1) {
      return {Model::get_therapy_db()[recurrent_therapy_id].get(), false};
    }
  }

  return {Model::get_treatment_strategy()->get_therapy(person), true};
}

void ProgressToClinicalEvent::apply_therapy(Person* person, Therapy* therapy,
                                            bool is_public_sector) {
  person->receive_therapy(therapy, clinical_caused_parasite_, false, is_public_sector);

  clinical_caused_parasite_->set_update_function(
      Model::get_instance()->having_drug_update_function());

  person->schedule_update_by_drug_event(clinical_caused_parasite_);
  // check if the person will progress to death despite of the treatment, this should be
  // 90% lower than the no treatment case
  if (person->will_progress_to_death_when_recieve_treatment()) {
    person->cancel_all_events_except(nullptr);
    person->set_host_state(Person::DEAD);
    Model::get_mdc()->record_1_malaria_death(person->get_location(), person->get_age(), true);

    person->schedule_report_treatment_failure_death_event(
        therapy->get_id(), Model::get_config()->get_therapy_parameters().get_tf_testing_day());
    return;
  }
}

void ProgressToClinicalEvent::do_execute() {
  // spdlog::info("ProgressToClinicalEvent::do_execute");
  auto* person = get_person();

  if (person == nullptr) { throw std::runtime_error("Person is nullptr"); }
  if (person->get_all_clonal_parasite_populations()->size() == 0) {
    // parasites might be cleaned by immune system or other things else
    return;
  }

  // if the clinical_caused_parasite eventually removed then do nothing
  if (!person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_)) {
    // spdlog::info("ProgressToClinicalEvent::do_execute: parasite removed");
    return;
  }

  if (person->get_host_state() == Person::CLINICAL) {
    // spdlog::info("ProgressToClinicalEvent::do_execute: Person is already Clinical");
    clinical_caused_parasite_->set_update_function(
        Model::get_instance()->immunity_clearance_update_function());
    return;
  }

  transition_to_clinical_state(person);
}

void ProgressToClinicalEvent::transition_to_clinical_state(Person* person) {
  const auto density =
      Model::get_random()->random_uniform<double>(Model::get_config()
                                                      ->get_parasite_parameters()
                                                      .get_parasite_density_levels()
                                                      .get_log_parasite_density_clinical_from(),
                                                  Model::get_config()
                                                      ->get_parasite_parameters()
                                                      .get_parasite_density_levels()
                                                      .get_log_parasite_density_clinical_to());

  clinical_caused_parasite_->set_last_update_log10_parasite_density(density);

  // Person change state to Clinical
  // Person change state to Clinical
  person->set_host_state(Person::CLINICAL);

  // If this clinical event is caused by the same parasite that has a pending
  // treatment-failure test, then this is an observed recurrent clinical episode
  // before the formal test day. Record treatment failure now.
  // Do not do this earlier when recurrence is merely scheduled.
  const bool is_recrudescence_before_tf_test =
    record_original_treatment_failure_if_recrudescence_before_tf_test(
        person,
        clinical_caused_parasite_
    );

  (void)is_recrudescence_before_tf_test;

  person->cancel_all_other_progress_to_clinical_events_except(this);
  int count = 0;
  std::string event_time = "";
  for (const auto& pair : person->get_events()) {
    if ( typeid(*pair.second).name() == typeid(ProgressToClinicalEvent).name()
     && pair.second->is_executable()) {
      event_time += std::to_string(pair.first) + " ";
      count++;
     }
  }
  if (count > 1) {
    spdlog::warn("Person {} has {} ProgressToClinicalEvent, time {} after canceling",
      person->get_age(), count, event_time);
  }

  person->change_all_parasite_update_function(
      Model::get_instance()->progress_to_clinical_update_function(),
      Model::get_instance()->immunity_clearance_update_function());

  clinical_caused_parasite_->set_update_function(Model::get_instance()->clinical_update_function());

  // Statistic collect cumulative clinical episodes
  Model::get_mdc()->collect_1_clinical_episode(person->get_location(), person->get_age(),
                                               person->get_age_class());

  if (should_receive_treatment(person)) {
    const auto [therapy, is_public_sector] =
        determine_therapy(person, is_recrudescence_before_tf_test);

    // Count every treated clinical episode.
    Model::get_mdc()->record_1_treatment(
        person->get_location(),
        person->get_age(),
        person->get_age_class(),
        therapy->get_id()
    );

    // Sub-counter: this treatment was given for a recrudescent/recurrent
    // clinical episode caused by the same parasite as the pending TF event.
    if (is_recrudescence_before_tf_test) {
      Model::get_mdc()->record_1_recrudescence_treatment(
          person->get_location(),
          person->get_age(),
          person->get_age_class(),
          therapy->get_id()
      );
    }

    // Schedule a new treatment-failure test for the treatment just given.
    person->schedule_test_treatment_failure_event(
        clinical_caused_parasite_,
        Model::get_config()->get_therapy_parameters().get_tf_testing_day(),
        therapy->get_id()
    );

    apply_therapy(person, therapy, is_public_sector);
  } else {
    handle_no_treatment(person);
  }
  // schedule end clinical event for both treatment and non-treatment cases
  person->schedule_end_clinical_event(clinical_caused_parasite_);
}
