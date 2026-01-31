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
  return density;
}
