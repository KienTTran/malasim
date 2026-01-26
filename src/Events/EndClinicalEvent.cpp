/*
 * File:   EndClinicalEvent.cpp
 * Author: Merlin
 *
 * Created on July 31, 2013, 12:27 PM
 */

#include "EndClinicalEvent.h"

#include "Core/Scheduler/Scheduler.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"

// OBJECTPOOL_IMPL(EndClinicalEvent)

void EndClinicalEvent::do_execute() {
  auto* person = get_person();

  if (person == nullptr) { throw std::runtime_error("Person is nullptr"); }

  if (person->get_all_clonal_parasite_populations()->size() == 0) {
    person->change_state_when_no_parasite_in_blood();

  } else {
    // still have parasite in blood
    person->get_immune_system()->set_increase(true);
    person->set_host_state(Person::ASYMPTOMATIC);

    if (person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_)) {
      person->determine_symptomatic_recrudescence(clinical_caused_parasite_);
    }
  }
}
