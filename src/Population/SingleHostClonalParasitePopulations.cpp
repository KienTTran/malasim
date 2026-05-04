#include "SingleHostClonalParasitePopulations.h"

#include <algorithm>
#include <cmath>

#include "ClonalParasitePopulation.h"
#include "Configuration/Config.h"
#include "DrugsInBlood.h"
#include "MDC/ModelDataCollector.h"
#include "Mosquito/Mosquito.h"
#include "Parasites/Genotype.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Treatment/Therapies/Drug.h"

using std::ranges::any_of;

SingleHostClonalParasitePopulations::SingleHostClonalParasitePopulations(Person* person)
    : person_(person) {}

void SingleHostClonalParasitePopulations::init() { parasites_.clear(); }

SingleHostClonalParasitePopulations::~SingleHostClonalParasitePopulations() {
  parasites_.clear();
  person_ = nullptr;
}

void SingleHostClonalParasitePopulations::clear() { parasites_.clear(); }

// transfer ownership of the parasite to the SingleHostClonalParasitePopulations
void SingleHostClonalParasitePopulations::add(
    std::unique_ptr<ClonalParasitePopulation> blood_parasite) {
  blood_parasite->set_parasite_population(this);
  // move the parasite to the vector
  parasites_.push_back(std::move(blood_parasite));
  parasites_.back()->set_index(parasites_.size() - 1);
}

// void SingleHostClonalParasitePopulations::remove(ClonalParasitePopulation* blood_parasite) {
//   if (blood_parasite != parasites_[blood_parasite->get_index()].get()) {
//     throw std::runtime_error(
//         "Incorrect index, or parasite not belong to this SingleHostClonalParasitePopulations when
//         " "remove parasite from SingleHostClonalParasitePopulations");
//   }
//   remove(blood_parasite->get_index());
// }

void SingleHostClonalParasitePopulations::remove(size_t index) {
  if (index < 0 || index >= parasites_.size()) { throw std::out_of_range("Index out of range"); }
  ClonalParasitePopulation* bp = parasites_[index].get();
  //    std::cout << parasites_.size() << std::endl;
  if (bp->get_index() != index) {
    spdlog::error(
        "Incorrect index when remove parasite from SingleHostClonalParasitePopulations: {} - {} - "
        "{}",
        bp->get_index(), index, parasites_[index]->get_index());
    throw std::runtime_error(
        "Incorrect index when remove parasite from SingleHostClonalParasitePopulations");
  }

  // v9: unregister from treated-parasite tracking before removal
  if (person_ != nullptr) {
    person_->unregister_treated_parasite(bp);
  }

  parasites_.back()->set_index(index);
  parasites_[index] = std::move(parasites_.back());
  parasites_.pop_back();
  // bp automatically deleted
}

int SingleHostClonalParasitePopulations::latest_update_time() const {
  return person_->get_latest_update_time();
}

bool SingleHostClonalParasitePopulations::contain(ClonalParasitePopulation* blood_parasite) {
  return std::ranges::any_of(parasites_, [blood_parasite](const auto &parasite) {
    if (!parasite) { throw std::runtime_error("Parasite is nullptr"); }
    return parasite.get() == blood_parasite;
  });
}

void SingleHostClonalParasitePopulations::change_all_parasite_update_function(
    ParasiteDensityUpdateFunction* from, ParasiteDensityUpdateFunction* to) const {
  for (const auto &parasite : parasites_) {
    if (parasite->update_function() == from) { parasite->set_update_function(to); }
  }
}

void SingleHostClonalParasitePopulations::update() {
  for (auto &bp : parasites_) {
    if (bp == nullptr) {
      throw std::runtime_error(
          "Parasite is nullptr in SingleHostClonalParasitePopulations::update");
    }
    bp->update();
  }
}

void SingleHostClonalParasitePopulations::clear_cured_parasites(double cured_threshold) {
  log10_total_infectious_density_ = ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY;

  // Check if the vector is empty before proceeding
  if (parasites_.empty()) {
    return;  // Nothing to clear or update
  }

  // Use a signed type for the index to avoid underflow issues with the loop condition
  for (int i = static_cast<int>(parasites_.size()) - 1; i >= 0; --i) {
    // Ensure index is valid before accessing (although the loop condition handles this)
    if (i < 0 || i >= static_cast<int>(parasites_.size())) {
      // This should ideally not happen with the corrected loop, but adds safety
      continue;
    }

    if (parasites_[i]->last_update_log10_parasite_density() <= cured_threshold) {
      remove(static_cast<size_t>(i));  // remove expects size_t
    } else {
      // Safely calculate total infectious density
      double log10_density = parasites_[i]->get_log10_infectious_density();
      if (log10_total_infectious_density_ == ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY) {
        log10_total_infectious_density_ = log10_density;
      } else {
        // Avoid potential floating point issues if density is very low
        if (log10_density > ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY) {
          log10_total_infectious_density_ +=
              log10(pow(10, log10_density - log10_total_infectious_density_) + 1.0);
        }
      }
    }
  }
}

void SingleHostClonalParasitePopulations::update_by_drugs(DrugsInBlood* drugs_in_blood) const {
  if (drugs_in_blood == nullptr) { throw std::invalid_argument("Drugs in blood is nullptr"); }
  for (const auto &blood_parasite : parasites_) {
    auto* new_genotype = blood_parasite->genotype();

    double percent_parasite_remove = 0;
    for (auto &[drug_id, drug] : *drugs_in_blood) {
      // select all locus
      // remember to use mask to turn on and off mutation location
      // for a specific time
      Genotype* candidate_genotype = new_genotype->perform_mutation_by_drug(
          Model::get_config(), Model::get_random(), drug->drug_type(),
          Model::get_config()->get_genotype_parameters().get_mutation_probability_per_locus());

      if (candidate_genotype->get_EC50_power_n(drug->drug_type())
          > new_genotype->get_EC50_power_n(drug->drug_type())) {
        // higher EC50^n means lower efficacy then allow mutation occur
        new_genotype = candidate_genotype;
      }
      if (new_genotype != blood_parasite->genotype()) {
        // if(blood_parasite->genotype()->get_aa_sequence()[35] == 'C'
        //    && new_genotype->get_aa_sequence()[35] == 'Y'){
        //   spdlog::info("580 C -> Y");
        // }
        // if(new_genotype->get_aa_sequence()[35] == 'Y'){
        //     spdlog::info("new 580Y {} ->
        //     {}",blood_parasite->genotype()->aa_sequence,new_genotype->aa_sequence);
        // }
        // mutation occurs
        Model::get_mdc()->record_1_mutation(person_->get_location(), blood_parasite->genotype(),
                                            new_genotype);
        Model::get_mdc()->record_1_mutation_by_drug(
            person_->get_location(), blood_parasite->genotype(), new_genotype, drug_id);

        //          LOG(TRACE) << Model::get_scheduler()->current_time() << "\t" <<
        //          blood_parasite->genotype()->genotype_id()
        //          << "\t"
        //                     << new_genotype->genotype_id() << "\t"
        //                     << blood_parasite->genotype()->get_EC50_power_n(drug->drug_type()) <<
        //                     "\t"
        //                     << new_genotype->get_EC50_power_n(drug->drug_type());
        blood_parasite->set_genotype(new_genotype);
      }

      const auto p_temp =
          drug->get_parasite_killing_rate(blood_parasite->genotype()->genotype_id());
      percent_parasite_remove = percent_parasite_remove + p_temp - percent_parasite_remove * p_temp;
    }
    if (percent_parasite_remove > 0) {
      blood_parasite->perform_drug_action(percent_parasite_remove,
                                          Model::get_config()
                                              ->get_parasite_parameters()
                                              .get_parasite_density_levels()
                                              .get_log_parasite_density_cured());
    }
  }
}

bool SingleHostClonalParasitePopulations::has_detectable_parasite(
    double detectable_threshold) const {
  return std::ranges::any_of(parasites_, [detectable_threshold](const auto &parasite) {
    if (!parasite) { throw std::runtime_error("Parasite is nullptr"); }
    return parasite->last_update_log10_parasite_density() >= detectable_threshold;
  });
}

bool SingleHostClonalParasitePopulations::is_gametocytaemic() const {
  for (const auto &parasite : parasites_) {
    if (parasite->gametocyte_level() > 0) { return true; }
  }
  return false;
}
