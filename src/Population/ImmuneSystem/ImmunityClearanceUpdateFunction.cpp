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

  // Immunity boost: daily exposure boost (clearance channel)
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (cfg.enable) {
    int current_time = Model::get_scheduler()->current_time();
    int parasite_in_blood_days = current_time - parasite->first_date_in_blood();
    // Apply when parasite has been in blood long enough for clearance exposure
    if (parasite_in_blood_days >= cfg.clearance.exposure_gate_days) {
      double multiplier = 1.0;
      // If drugs are present, scale the amount by drug_exposure_multiplier
      if (p->has_effective_drug_in_blood()) {
        multiplier = cfg.drug_exposure_multiplier;
      }
      p->get_immune_system()->add_daily_clearance_exposure_boost(current_time, multiplier);
    }
  }

  return density;
}
