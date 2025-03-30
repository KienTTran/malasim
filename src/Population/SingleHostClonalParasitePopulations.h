#ifndef SINGLEHOSTCLONALPARASITEPOPULATIONS_H
#define SINGLEHOSTCLONALPARASITEPOPULATIONS_H

#include <vector>
#include <memory>

#include "Utils/TypeDef.hxx"
#include "Population/ClonalParasitePopulation.h"
class ClonalParasitePopulation;

class Person;

class DrugType;

class DrugsInBlood;

class ParasiteDensityUpdateFunction;

class SingleHostClonalParasitePopulations {
  // OBJECTPOOL(SingleHostClonalParasitePopulations)

  //disallow copy and assign
  SingleHostClonalParasitePopulations(const SingleHostClonalParasitePopulations &) = delete;
  void operator=(const SingleHostClonalParasitePopulations &) = delete;

private:
  Person* person_{nullptr};
  std::vector<std::unique_ptr<ClonalParasitePopulation>> parasites_;

public:
  // Use constexpr for compile-time constant
  static constexpr double DEFAULT_LOG_DENSITY = ClonalParasitePopulation::LOG_ZERO_PARASITE_DENSITY;
  double log10_total_infectious_denstiy{DEFAULT_LOG_DENSITY};

  // Use [[nodiscard]] consistently and reference member functions that don't modify state as const
  [[nodiscard]] double log10_total_infectious_density() const noexcept {
    return log10_total_infectious_denstiy;
  }

  void set_log10_total_infectious_density(double value) noexcept {  // Pass by value for primitives
    log10_total_infectious_denstiy = value;
  }

  [[nodiscard]] Person* person() const noexcept {
    return person_;
  }

  void set_person(Person* value) noexcept {
    person_ = value;
  }

  [[nodiscard]] const std::vector<std::unique_ptr<ClonalParasitePopulation>>& parasites() const {
    return parasites_;
  }

public:
  // Use explicit for single-parameter constructors
  explicit SingleHostClonalParasitePopulations(Person* person = nullptr);
  
  // Mark destructor as default if it doesn't need special handling
  virtual ~SingleHostClonalParasitePopulations();

  // Mark virtual functions that are meant to be overridden with override in derived classes
  [[nodiscard]] virtual int size() const noexcept;  // Add const if not modifying state

  void init();

  virtual void add(ClonalParasitePopulation *blood_parasite);

  virtual void remove(ClonalParasitePopulation *blood_parasite);

  virtual void remove(const int &index);

  virtual int latest_update_time() const;

  virtual bool contain(ClonalParasitePopulation *blood_parasite);

  void change_all_parasite_update_function(ParasiteDensityUpdateFunction *from,
                                           ParasiteDensityUpdateFunction *to) const;

  void update();

  void clear_cured_parasites();

  void clear();

  void update_by_drugs(DrugsInBlood *drugs_in_blood) const;

  bool has_detectable_parasite() const;

  bool is_gametocytaemic() const;
};

#endif /* SINGLEHOSTCLONALPARASITEPOPULATIONS_H */
