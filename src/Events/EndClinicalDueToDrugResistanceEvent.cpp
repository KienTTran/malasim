#include "EndClinicalDueToDrugResistanceEvent.h"

#include "Configuration/Config.h"
#include "Utils/Random.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/DrugsInBlood.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"

//OBJECTPOOL_IMPL(EndClinicalDueToDrugResistanceEvent)

EndClinicalDueToDrugResistanceEvent::EndClinicalDueToDrugResistanceEvent()
    : clinical_caused_parasite_(nullptr) {}

EndClinicalDueToDrugResistanceEvent::~EndClinicalDueToDrugResistanceEvent() =
    default;

void EndClinicalDueToDrugResistanceEvent::schedule_event(
    Scheduler* scheduler, Person* p,
    ClonalParasitePopulation* clinical_caused_parasite, const int &time) {
  if (scheduler != nullptr) {
    auto* e = new EndClinicalDueToDrugResistanceEvent();
    e->dispatcher = p;
    e->set_clinical_caused_parasite(clinical_caused_parasite);
    e->time = time;

    p->add_dispatcher(e);
    scheduler->schedule_individual_event(e);
  }
}

void EndClinicalDueToDrugResistanceEvent::execute() {
  auto* person = dynamic_cast<Person*>(dispatcher);
  if (person->get_all_clonal_parasite_populations()->size() == 0) {
    person->change_state_when_no_parasite_in_blood();

  } else {
    // still have parasite in blood
    person->get_immune_system()->set_increase(true);
    person->set_host_state(Person::ASYMPTOMATIC);

    if (person->get_all_clonal_parasite_populations()->contain(
            clinical_caused_parasite_)) {
      clinical_caused_parasite_->set_last_update_log10_parasite_density(
          Model::get_instance().get_config()->get_parasite_parameters().get_parasite_density_levels()
              .get_log_parasite_density_asymptomatic());

      person->determine_relapse_or_not(clinical_caused_parasite_);
    }

    //        person->determine_relapse_or_not(clinical_caused_parasite_);
    //        person->determine_clinical_or_not(clinical_caused_parasite_);

    //        if
    //        (clinical_caused_parasite_->last_update_log10_parasite_density() <
    //        Model::CONFIG->parasite_density_level().log_parasite_density_asymptomatic)
    //        {
    //            std::cout <<
    //            clinical_caused_parasite_->last_update_log10_parasite_density()<<
    //            std::endl; assert(false);
    //        }
    //        clinical_caused_parasite_->set_last_update_log10_parasite_density(Model::CONFIG->parasite_density_level().log_parasite_density_asymptomatic);
    //        clinical_caused_parasite_->set_update_function(Model::MODEL->immunity_clearance_update_function());
    //        //        std::cout << "hello" << std::endl;
  }
}
