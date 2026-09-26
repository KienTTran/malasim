#ifndef CLONALPARASITEPOPULATION_H
#define CLONALPARASITEPOPULATION_H

#include <cstdint>

#include "Core/types.h"
#include "ParasiteDensity/ParasiteDensityUpdateFunction.h"
#include "Treatment/Therapies/DrugType.h"
#include "Utils/Index/Indexer.h"

class Therapy;

class Genotype;

class SingleHostClonalParasitePopulations;

class ClonalParasitePopulation : public utils::Indexer {
  // OBJECTPOOL(ClonalParasitePopulation);
public:
  // disallow copy and assign

  ClonalParasitePopulation(ClonalParasitePopulation &&) = delete;
  ClonalParasitePopulation &operator=(ClonalParasitePopulation &&) = delete;
  ClonalParasitePopulation(const ClonalParasitePopulation &) = delete;
  ClonalParasitePopulation &operator=(const ClonalParasitePopulation &) = delete;
  explicit ClonalParasitePopulation(Genotype* genotype = nullptr);
  ~ClonalParasitePopulation() override;

  static constexpr double LOG_ZERO_PARASITE_DENSITY = -1000;

  // Unique, never-reused id. Events keep a raw pointer to "their" parasite; after
  // that parasite is freed the allocator can hand the same address to a new
  // parasite, so pointer equality alone can match the wrong clone. Compare the
  // uid as well (see SingleHostClonalParasitePopulations::contain(ptr, uid)).
  [[nodiscard]] std::uint64_t uid() const noexcept { return uid_; }

  // True while this clone is waiting for the clinical decision it skipped
  // because it reached the blood under an effective drug.
  [[nodiscard]] bool pending_breakthrough_check() const noexcept {
    return pending_breakthrough_check_;
  }
  void set_pending_breakthrough_check(const bool value) noexcept {
    pending_breakthrough_check_ = value;
  }

  [[nodiscard]] double last_update_log10_parasite_density() const noexcept {
    return last_update_log10_parasite_density_;
  }
  void set_last_update_log10_parasite_density(const double &value) noexcept {
    last_update_log10_parasite_density_ = value;
  }

  [[nodiscard]] double gametocyte_level() const noexcept { return gametocyte_level_; }
  void set_gametocyte_level(const double &value) noexcept { gametocyte_level_ = value; }

  [[nodiscard]] core::SimDay first_date_in_blood() const { return first_date_in_blood_; }
  void set_first_date_in_blood(const core::SimDay &value) { first_date_in_blood_ = value; }

  [[nodiscard]] Genotype* genotype() const noexcept { return genotype_; }
  void set_genotype(Genotype* value) noexcept { genotype_ = value; }

  [[nodiscard]] ParasiteDensityUpdateFunction* update_function() const noexcept {
    return update_function_;
  }
  void set_update_function(ParasiteDensityUpdateFunction* value) { update_function_ = value; }
  [[nodiscard]] SingleHostClonalParasitePopulations* parasite_population() {
    return parasite_population_;
  }
  void set_parasite_population(SingleHostClonalParasitePopulations* value) {
    parasite_population_ = value;
  }

  double get_current_parasite_density(const int &current_time);

  [[nodiscard]] double get_log10_infectious_density() const;

  [[nodiscard]] bool resist_to(const int &drug_id) const;

  void update();

  void perform_drug_action(double percent_parasite_remove, double log10_parasite_density_cured);

private:
  SingleHostClonalParasitePopulations* parasite_population_{nullptr};
  // Non-owning pointer. SingleHostClonalParasitePopulations owns all ClonalParasitePopulation
  // instances.
  Genotype* genotype_{nullptr};
  // Non-owning pointer
  ParasiteDensityUpdateFunction* update_function_{nullptr};

  double last_update_log10_parasite_density_{LOG_ZERO_PARASITE_DENSITY};
  double gametocyte_level_{0.0};

  core::SimDay first_date_in_blood_{core::K_INVALID_SIM_DAY};

  std::uint64_t uid_{0};
  bool pending_breakthrough_check_{false};
  static inline std::uint64_t next_uid_{1};
};

#endif /* CLONALPARASITEPOPULATION_H */

