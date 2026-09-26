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

double log10_sum(double log10_a, double log10_b);

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
  if (index >= parasites_.size()) { throw std::out_of_range("Index out of range"); }

  ClonalParasitePopulation* bp = parasites_[index].get();

  if (bp->get_index() != index) {
    spdlog::error(
        "Incorrect index when remove parasite from "
        "SingleHostClonalParasitePopulations: {} - {} - {}",
        bp->get_index(), index, parasites_[index]->get_index());

    throw std::runtime_error(
        "Incorrect index when remove parasite from "
        "SingleHostClonalParasitePopulations");
  }

  const size_t last_index = parasites_.size() - 1;

  if (index != last_index) {
    parasites_.back()->set_index(index);
    parasites_[index] = std::move(parasites_.back());
  }

  parasites_.pop_back();
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

bool SingleHostClonalParasitePopulations::contain(ClonalParasitePopulation* blood_parasite,
                                                  std::uint64_t uid) {
  if (blood_parasite == nullptr) { return false; }
  return std::ranges::any_of(parasites_, [blood_parasite, uid](const auto &parasite) {
    if (!parasite) { throw std::runtime_error("Parasite is nullptr"); }
    return parasite.get() == blood_parasite && parasite->uid() == uid;
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

void SingleHostClonalParasitePopulations::update_with_drug_effects(DrugsInBlood* drugs_in_blood) {
  if (drugs_in_blood == nullptr) { throw std::invalid_argument("Drugs in blood is nullptr"); }

  for (auto &blood_parasite : parasites_) {
    if (blood_parasite == nullptr) {
      throw std::runtime_error(
          "Parasite is nullptr in SingleHostClonalParasitePopulations::update_with_drug_effects");
    }
    blood_parasite->update();
    apply_drug_effects_to(blood_parasite.get(), drugs_in_blood);
    apply_cnv_reversion_to(blood_parasite.get(), drugs_in_blood);
  }
}

void SingleHostClonalParasitePopulations::update_with_drug_effects_and_clear_cured(
    DrugsInBlood* drugs_in_blood, double cured_threshold) {
  if (drugs_in_blood == nullptr) { throw std::invalid_argument("Drugs in blood is nullptr"); }

  log10_total_infectious_density_ = ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY;

  size_t p_index = 0;
  while (p_index < parasites_.size()) {
    auto &blood_parasite = parasites_[p_index];
    if (blood_parasite == nullptr) {
      throw std::runtime_error(
          "Parasite is nullptr in "
          "SingleHostClonalParasitePopulations::update_with_drug_effects_and_clear_cured");
    }

    blood_parasite->update();
    apply_drug_effects_to(blood_parasite.get(), drugs_in_blood);
    apply_cnv_reversion_to(blood_parasite.get(), drugs_in_blood);

    if (blood_parasite->last_update_log10_parasite_density() <= cured_threshold) {
      remove(p_index);
      continue;
    }

    log10_total_infectious_density_ =
        log10_sum(log10_total_infectious_density_,
                  blood_parasite->get_log10_infectious_density());
    ++p_index;
  }
}

double log10_sum(double log10_a, double log10_b) {
  const double zero = ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY;

  if (log10_a <= zero) { return log10_b; }

  if (log10_b <= zero) { return log10_a; }

  const double max_log = std::max(log10_a, log10_b);
  const double min_log = std::min(log10_a, log10_b);

  return max_log + std::log10(1.0 + std::pow(10.0, min_log - max_log));
}

void SingleHostClonalParasitePopulations::clear_cured_parasites(double cured_threshold) {
  log10_total_infectious_density_ = ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY;

  size_t p_index = 0;
  while (p_index < parasites_.size()) {
    if (parasites_[p_index]->last_update_log10_parasite_density() <= cured_threshold) {
      remove(p_index);

      // Do not increment i.
      // A parasite from the back may have been moved into this position.
      continue;
    }

    log10_total_infectious_density_ = log10_sum(
        log10_total_infectious_density_, parasites_[p_index]->get_log10_infectious_density());

    ++p_index;
  }
}

void SingleHostClonalParasitePopulations::apply_drug_effects_to(
    ClonalParasitePopulation* blood_parasite, DrugsInBlood* drugs_in_blood) const {
  if (drugs_in_blood == nullptr) { throw std::invalid_argument("Drugs in blood is nullptr"); }
  if (drugs_in_blood->size() == 0) { return; }

  auto* config = Model::get_config();
  auto* random = Model::get_random();
  const auto mutation_probability =
      config->get_genotype_parameters().get_mutation_probability_per_locus();
  const auto log_parasite_density_cured = config->get_parasite_parameters()
                                              .get_parasite_density_levels()
                                              .get_log_parasite_density_cured();

  auto* new_genotype = blood_parasite->genotype();

  double percent_parasite_remove = 0;
  for (auto &[drug_id, drug] : *drugs_in_blood) {
    // select all locus
    // remember to use mask to turn on and off mutation location
    // for a specific time
    auto* candidate_genotype = new_genotype->perform_mutation_by_drug(
        config, random, drug->drug_type(), mutation_probability);

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

    const auto p_temp = drug->get_parasite_killing_rate(blood_parasite->genotype()->genotype_id());
    percent_parasite_remove = percent_parasite_remove + p_temp - (percent_parasite_remove * p_temp);
  }
  if (percent_parasite_remove > 0) {
    blood_parasite->perform_drug_action(percent_parasite_remove, log_parasite_density_cured);
  }
}

void SingleHostClonalParasitePopulations::apply_cnv_reversion_to(
    ClonalParasitePopulation* blood_parasite, DrugsInBlood* drugs_in_blood) const {
  auto* reverted_genotype = blood_parasite->genotype()->perform_cnv_reversion(
      Model::get_config(), Model::get_random(), drugs_in_blood);
  if (reverted_genotype == blood_parasite->genotype()) { return; }

  Model::get_mdc()->record_1_mutation(person_->get_location(), blood_parasite->genotype(),
                                      reverted_genotype);
  blood_parasite->set_genotype(reverted_genotype);
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
