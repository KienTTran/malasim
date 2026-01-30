#include "ImmunityClearanceUpdateFunction.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Population/Person/Person.h"
#include "ImmuneSystem.h"
#include "Simulation/Model.h"
#include "Parasites/Genotype.h"

ImmunityClearanceUpdateFunction::ImmunityClearanceUpdateFunction(Model *model) : model_(model) {}

ImmunityClearanceUpdateFunction::~ImmunityClearanceUpdateFunction() = default;

double ImmunityClearanceUpdateFunction::get_current_parasite_density(ClonalParasitePopulation *parasite, int duration) {

  auto *p = parasite->parasite_population()->person();
  double density = p->get_immune_system()->get_parasite_size_after_t_days(duration, parasite->last_update_log10_parasite_density(),
                                                            parasite->genotype()->daily_fitness_multiple_infection);

  // Immunity boost: daily exposure boost
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  // NOTE: Apply daily exposure boost whenever the parasite has been in the blood
  // for long enough (exposure_gate_days). Previously this skipped when any
  // drugs were present in the host; that condition was removed to match the
  // configuration/comment change and updated behavior.
  if (cfg.enable) {
    int current_time = Model::get_scheduler()->current_time();
    int parasite_in_blood_days = current_time - parasite->first_date_in_blood();
    if (parasite_in_blood_days >= cfg.exposure_gate_days) {
      p->get_immune_system()->add_daily_exposure_boost(current_time);
    }
  }

  return density;
}
