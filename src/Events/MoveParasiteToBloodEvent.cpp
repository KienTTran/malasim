#include "MoveParasiteToBloodEvent.h"


#include "Configuration/Config.h"
#include "Utils/Random.h"
#include "Core/Scheduler/Scheduler.h"
#include "Simulation/Model.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/DrugsInBlood.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Treatment/Therapies/Drug.h"

//OBJECTPOOL_IMPL(MoveParasiteToBloodEvent)

void MoveParasiteToBloodEvent::do_execute() {
  auto* person = get_person();
  if (person == nullptr) {
    throw std::runtime_error("Person is nullptr");
  }
  
  auto* parasite_type = person->liver_parasite_type();
  person->set_liver_parasite_type(nullptr);

  // add to blood
  if (person->get_host_state() == Person::EXPOSED) {
    person->set_host_state(Person::ASYMPTOMATIC);
  }

  person->get_immune_system()->set_increase(true);

  auto new_parasite = person->add_new_parasite_to_blood(parasite_type);

  // Set per-person first blood exposure day if not already set. This marks the day
  // the host first had a blood-stage parasite and is used by host-level daily learning.
  if (person->get_first_blood_exposure_day() < 0) {
    person->set_first_blood_exposure_day(Model::get_scheduler()->current_time());
  }

  new_parasite->set_last_update_log10_parasite_density(
      Model::get_random()->random_normal_truncated(
          Model::get_config()->get_parasite_parameters().get_parasite_density_levels()
              .get_log_parasite_density_asymptomatic(),
          0.5));

  if (person->has_effective_drug_in_blood()) {
    // spdlog::info("Person has drug in blood");
    // person has drug in blood
    new_parasite->set_update_function(
        Model::get_instance()->having_drug_update_function());
  } else {
    // spdlog::info("Person does not have drug in blood");
    if (person->get_all_clonal_parasite_populations()->size() > 1) {
      // spdlog::info("person->get_all_clonal_parasite_populations()->size() > 1");
      if (Model::get_config()->get_epidemiological_parameters().get_allow_new_coinfection_to_cause_symptoms()) {
        person->determine_clinical_or_not(new_parasite);
      } else {
        new_parasite->set_update_function(
            Model::get_instance()->immunity_clearance_update_function());
      }
    } else {
      // spdlog::info("person->get_all_clonal_parasite_populations()->size() <= 1");
      person->determine_clinical_or_not(new_parasite);
    }
  }

  person->schedule_mature_gametocyte_event(new_parasite);
}
