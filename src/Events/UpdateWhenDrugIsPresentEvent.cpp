#include "UpdateWhenDrugIsPresentEvent.h"

#include "Population/Person/Person.h"
#include "Population/ClonalParasitePopulation.h"
#include "Core/Scheduler/Scheduler.h"
#include "Population/DrugsInBlood.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Configuration/Config.h"
#include "Simulation/Model.h"
#include "Treatment/Therapies/Drug.h"
//OBJECTPOOL_IMPL(UpdateWhenDrugIsPresentEvent)

void UpdateWhenDrugIsPresentEvent::do_execute() {
  auto *person = get_person();
  if (person == nullptr) {
    throw std::runtime_error("Person is nullptr");
  }
  // Only keep following the clone if it is still the same clone in this host;
  // the stored pointer may dangle (and must not be dereferenced) otherwise.
  auto* live_parasite =
      person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_,
                                                             clinical_caused_parasite_uid_)
          ? clinical_caused_parasite_
          : nullptr;
  if (person->drugs_in_blood()->size() > 0) {
    if (live_parasite != nullptr && person->get_host_state()==
        Person::CLINICAL) {
      if (clinical_caused_parasite_->last_update_log10_parasite_density() <= Model::get_config()->get_parasite_parameters().
          get_parasite_density_levels().
          get_log_parasite_density_asymptomatic()) {
        person->set_host_state(Person::ASYMPTOMATIC);
      }
    }
    person->schedule_update_by_drug_event(live_parasite);
  } else {
    for (auto i = 0; i < person->get_all_clonal_parasite_populations()->size(); i++) {
      auto* blood_parasite = person->get_all_clonal_parasite_populations()->at(i);
      if (blood_parasite->update_function()==Model::get_instance()->having_drug_update_function()) {
        blood_parasite->set_update_function(Model::get_instance()->immunity_clearance_update_function());
      }
    }
  }
}
