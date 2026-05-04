#include "TestTreatmentFailureEvent.h"

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Simulation/Model.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person/Person.h"

//OBJECTPOOL_IMPL(TestTreatmentFailureEvent)


void TestTreatmentFailureEvent::do_execute() {
  auto* person = get_person();
  if (person == nullptr) {
    throw std::runtime_error("Person is nullptr");
  }

  const bool parasite_present =
      clinical_caused_parasite_ != nullptr &&
      person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_);

  const double detectable_threshold =
      Model::get_config()
          ->get_parasite_parameters()
          .get_parasite_density_levels()
          .get_log_parasite_density_detectable();

  const bool parasite_detectable =
      parasite_present &&
      clinical_caused_parasite_->last_update_log10_parasite_density() > detectable_threshold;

  if (parasite_detectable) {
    Model::get_mdc()->record_1_treatment_failure_by_therapy(
        person->get_location(),
        person->get_age_class(),
        therapy_id_);
  } else {
    Model::get_mdc()->record_1_treatment_success_by_therapy(
        person->get_location(),
        person->get_age_class(),
        therapy_id_);
  }
}
