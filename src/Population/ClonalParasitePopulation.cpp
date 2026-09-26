#include "ClonalParasitePopulation.h"

#include <algorithm>
#include <cmath>

#include "Core/Scheduler/Scheduler.h"
#include "Parasites/Genotype.h"
#include "Simulation/Model.h"
#include "SingleHostClonalParasitePopulations.h"

ClonalParasitePopulation::ClonalParasitePopulation(Genotype* genotype)
    : genotype_(genotype), uid_(next_uid_++) {}

ClonalParasitePopulation::~ClonalParasitePopulation() = default;

double ClonalParasitePopulation::get_current_parasite_density(const int &current_time) {
  if (update_function_ == nullptr) { return last_update_log10_parasite_density_; }

  const auto duration = current_time - parasite_population()->latest_update_time();
  if (duration == 0) { return last_update_log10_parasite_density_; }

  if (duration < 0) {
    // duration is negative which is some error in the system
    // we do not allow thing happens in future
    spdlog::error("Duration is negative: {}", duration);
    throw std::invalid_argument("Duration is negative");
  }

  return update_function_->get_current_parasite_density(this, duration);
}

double ClonalParasitePopulation::get_log10_infectious_density() const {
  if (last_update_log10_parasite_density_ == LOG_ZERO_PARASITE_DENSITY
      || gametocyte_level_ == 0.0)
    return LOG_ZERO_PARASITE_DENSITY;

  return last_update_log10_parasite_density_ + log10(gametocyte_level_);
}

bool ClonalParasitePopulation::resist_to(const int &drug_id) const {
  return genotype_->resist_to(Model::get_drug_db()->at(drug_id).get());
}

void ClonalParasitePopulation::update() {
  set_last_update_log10_parasite_density(
      get_current_parasite_density(Model::get_scheduler()->current_time()));
}

void ClonalParasitePopulation::perform_drug_action(double percent_parasite_remove,
                                                   double log10_parasite_density_cured) {
  if (percent_parasite_remove < 0) {
    throw std::invalid_argument("Percent parasite remove is less than 0");
  }

  double new_size = last_update_log10_parasite_density_;
  if (percent_parasite_remove >= 1) {
    new_size = log10_parasite_density_cured;
  } else {
    new_size += log10(1 - percent_parasite_remove);
  }

  new_size = std::max(new_size, log10_parasite_density_cured);

  set_last_update_log10_parasite_density(new_size);
}
