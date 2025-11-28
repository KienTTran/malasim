/* 
 * File:   EndClinicalByNoTreatmentEvent.cpp
 * Author: Merlin
 * 
 * Created on July 31, 2013, 12:28 PM
 */

#include "EndClinicalByNoTreatmentEvent.h"
#include "Population/Person/Person.h"
#include "Population/ClonalParasitePopulation.h"
#include "Core/Scheduler/Scheduler.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Simulation/Model.h"
#include "Configuration/Config.h"
#include "Utils/Random.h"

void EndClinicalByNoTreatmentEvent::do_execute() {
  auto *person = get_person();

  if (person->get_all_clonal_parasite_populations()->empty()) {
    //        assert(false);
    person->change_state_when_no_parasite_in_blood();

  } else {
    //still have parasite in blood
    person->get_immune_system()->set_increase(true);
    person->set_host_state(Person::ASYMPTOMATIC);
    if (person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_)) {
      clinical_caused_parasite_->set_last_update_log10_parasite_density(
          Model::get_config()->get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_asymptomatic());

      person->determine_relapse_or_not(clinical_caused_parasite_);

    }
    //        std::cout << clinical_caused_parasite_->last_update_log10_parasite_density()<< std::endl;
    //        std::cout << person->immune_system()->get_lastest_immune_value()<< std::endl;
  }
}
