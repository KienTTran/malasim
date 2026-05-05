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
#include "Events/EndClinicalEvent.h"
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
#include "Utils/Random.h"

// OBJECTPOOL_IMPL(ProgressToClinicalEvent)

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

std::pair<Therapy*, bool> ProgressToClinicalEvent::determine_therapy(Person* person,
                                                                     bool is_recurrence) {
  auto* strategy = dynamic_cast<NestedMFTStrategy*>(Model::get_treatment_strategy());
  if (strategy != nullptr) {
    // if the strategy is NestedMFT and the therapy is the public sector
    const auto probability = Model::get_random()->random_flat(0.0, 1.0);

    double sum = 0;
    std::size_t s_id = -1;
    for (std::size_t i = 0; i < strategy->distribution.size(); i++) {
      sum += strategy->distribution[i];
      if (probability <= sum) {
        s_id = i;
        break;
      }
    }
    // this is public sector
    if (s_id == 0) {
      if (is_recurrence
          && Model::get_config()->get_therapy_parameters().get_recurrent_therapy_id() != -1) {
        return {Model::get_therapy_db()
                    [Model::get_config()->get_therapy_parameters().get_recurrent_therapy_id()]
                        .get(),
                false};
      }
      return {strategy->strategy_list[s_id]->get_therapy(person), true};
    }
    return {strategy->strategy_list[s_id]->get_therapy(person), false};
  }
  // If the strategy is not NestedMFT, then we need to handle the case when the recurrence therapy
  // id is not -1
  auto recurrent_therapy_id =
      Model::get_config()->get_therapy_parameters().get_recurrent_therapy_id();
  if (recurrent_therapy_id != -1) {
    return {Model::get_therapy_db()[recurrent_therapy_id].get(), false};
  }
  // If the strategy is not NestedMFT and the recurrence therapy id is -1, then we need to return
  // the first therapy in the strategy
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

  // Fix 1: person may have died (e.g. from a concurrent event on the same tick)
  // before this ProgressToClinicalEvent fires.  Skip silently.
  if (person->get_host_state() == Person::DEAD) { return; }

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
  person->set_host_state(Person::CLINICAL);

  // Cancel competing ProgressToClinicalEvents from other infections.
  person->cancel_all_other_progress_to_clinical_events_except(this);

  // Fix 2 (root cause): cancel any stale EndClinicalEvent that belongs to a
  // previous clinical episode.  Without this, a leftover EndClinicalEvent can
  // fire while the new episode is active and (a) forcibly set the person
  // CLINICAL→ASYMPTOMATIC, prematurely ending the current episode, and (b)
  // trigger determine_symptomatic_recrudescence for the old parasite, opening
  // a spurious extra recurrence pathway.
  // A fresh EndClinicalEvent is scheduled for this episode at the end of this
  // function, so cancelling old ones here is always safe.
  person->cancel_all_events<EndClinicalEvent>();

  person->change_all_parasite_update_function(
      Model::get_instance()->progress_to_clinical_update_function(),
      Model::get_instance()->immunity_clearance_update_function());

  clinical_caused_parasite_->set_update_function(Model::get_instance()->clinical_update_function());

  // Statistic collect cumulative clinical episodes
  Model::get_mdc()->collect_1_clinical_episode(person->get_location(), person->get_age(),
                                               person->get_age_class());

  if (should_receive_treatment(person)) {
    const auto [therapy, is_public_sector] = determine_therapy(person, false);

    Model::get_mdc()->record_1_treatment(person->get_location(), person->get_age(),
                                         person->get_age_class(), therapy->get_id());

    person->schedule_test_treatment_failure_event(
        clinical_caused_parasite_,
        Model::get_config()->get_therapy_parameters().get_tf_testing_day(), therapy->get_id());
    apply_therapy(person, therapy, is_public_sector);
  } else {
    handle_no_treatment(person);
  }

  // Fix 1: do NOT schedule an EndClinicalEvent when the person died inside
  // apply_therapy / handle_no_treatment.  An orphaned EndClinicalEvent on a
  // dead person calls change_state_when_no_parasite_in_blood (parasites were
  // cleared by set_host_state(DEAD)) which transitions DEAD→SUSCEPTIBLE,
  // resurrecting the person and allowing extra clinical episodes.
  if (person->get_host_state() == Person::DEAD) { return; }

  person->schedule_end_clinical_event(clinical_caused_parasite_);
}

// TODO: remove this code
//   const auto p = Model::get_random()->random_flat(0.0, 1.0);

//   const auto p_treatment = Model::get_treatment_coverage()->get_probability_to_be_treated(
//       person->get_location(), person->get_age());

// //   std::cout << p_treatment << std::endl;
//   if (p <= p_treatment) {
//     auto *therapy = Model::get_treatment_strategy()->get_therapy(person);

//     person->receive_therapy(therapy, clinical_caused_parasite_);
//     //Statistic increase today treatments
//     Model::get_mdc()->record_1_treatment(person->get_location(), person->get_age(),
//     person->get_age_class(), therapy->get_id());

//     clinical_caused_parasite_->set_update_function(Model::get_instance()->having_drug_update_function());

//     // calculate EAMU
//     Model::get_mdc()->record_AMU_AFU(person, therapy, clinical_caused_parasite_);
//     //        calculateEAMU(therapy);
//     //

//     // death is 90% lower than no treatment
//     if (person->will_progress_to_death_when_recieve_treatment()) {

//       //for each test treatment failure event inside individual
//       // record treatment failure (not tf)
//       //            person->record_treatment_failure_for_test_treatment_failure_events();

//       //no treatment routine
//       receive_no_treatment_routine(person);

//       person->cancel_all_events_except(nullptr);
//       person->set_host_state(Person::DEAD);
//       Model::get_mdc()->record_1_malaria_death(person->get_location(), person->get_age(),true);
//       Model::get_mdc()->record_1_TF(person->get_location(), true);
//       Model::get_mdc()->record_1_treatment_failure_by_therapy(person->get_location(),
//       person->get_age(),
//                                                                    therapy->get_id());
//       return;
//     }

//     person->schedule_update_by_drug_event(clinical_caused_parasite_);

//     person->schedule_end_clinical_event(clinical_caused_parasite_);
//     person->schedule_test_treatment_failure_event(clinical_caused_parasite_,
//     Model::get_config()->get_therapy_parameters().get_tf_testing_day(),
//                                                   therapy->get_id());

//   } else {
//     //not recieve treatment
//     //Statistic store NTF
//     Model::get_mdc()->record_1_TF(person->get_location(), false);
//     Model::get_mdc()->record_1_non_treated_case(person->get_location(), person->get_age(),
//     person->get_age_class());

//     receive_no_treatment_routine(person);
//     if (person->get_host_state()==Person::DEAD) {
//       Model::get_mdc()->record_1_malaria_death(person->get_location(), person->get_age(),false);
//       return;
//     }
//     //
//     //        //schedule for end of clinical event
//     //        std::cout << "progress clinical event" << std::endl;

//     person->schedule_end_clinical_by_no_treatment_event(clinical_caused_parasite_);
//   }

// void ProgressToClinicalEvent::receive_no_treatment_routine(Person *p) {
//   // if (p->will_progress_to_death_when_receive_no_treatment()) {
//   //   p->cancel_all_events_except(nullptr);
//   //   p->set_host_state(Person::DEAD);
//   // }
// }
