#ifndef IMMUNESYSTEM_H
#define IMMUNESYSTEM_H

#include <memory>
#include <vector>

#include "Utils/TypeDef.h"

class Model;

class Person;

class ImmuneComponent;

class Config;

// typedef std::vector<ImmuneComponent*> ImmuneComponentPtrVector;

class ImmuneSystem {
  // OBJECTPOOL(ImmuneSystem)
public:
  // Disallow copy
  ImmuneSystem(const ImmuneSystem&) = delete;
  ImmuneSystem& operator=(const ImmuneSystem&) = delete;

  explicit ImmuneSystem(Person* person = nullptr);

  virtual ~ImmuneSystem();

  [[nodiscard]] Person* person() const { return person_; }
  void set_person(Person* person) { person_ = person; }

  [[nodiscard]] ImmuneComponent* immune_component() const;
  void set_immune_component(std::unique_ptr<ImmuneComponent> value);

  [[nodiscard]] bool increase() const { return increase_; }
  void set_increase(bool increase) { increase_ = increase; }

  virtual void draw_random_immune();

  virtual void update();

  [[nodiscard]] virtual double get_latest_immune_value() const;

  virtual void set_latest_immune_value(double value);

  [[nodiscard]] virtual double get_current_value() const;

  [[nodiscard]] virtual double get_parasite_size_after_t_days(const int &duration,
                                                              const double &original_size,
                                                              const double &fitness) const;

  [[nodiscard]] virtual double get_clinical_progression_probability() const;

  // Immunity boost methods
  void update_extra_boost_decay(int current_day);
  void add_extra_boost(double amount, int current_day);
  void add_daily_exposure_boost(int current_day);
  [[nodiscard]] double get_effective_immunity(int current_day) const;

  // Two-channel API: clinical (affects clinical progression) and clearance (affects parasite control)
  void update_boosts_decay_for_day(int current_day);
  void add_clinical_event_boost(double amount, int current_day);
  void add_clearance_event_boost(double amount, int current_day);
  void add_daily_clearance_exposure_boost(int current_day, double amount_multiplier = 1.0);
  void add_daily_clinical_exposure_boost(int current_day, double amount_multiplier = 1.0);

  [[nodiscard]] double get_effective_clinical_immunity(int current_day) const;
  [[nodiscard]] double get_effective_clearance_immunity(int current_day) const;

private:
  Person* person_{nullptr};
  std::unique_ptr<ImmuneComponent> immune_component_{nullptr};
  bool increase_{false};

  // Extra boost state (legacy single-channel kept for backward compatibility)
  double extra_boost_{0.0};
  int last_extra_boost_update_day_{-1};
  int last_daily_boost_day_{-1};

  // New two-channel boost state
  struct BoostChannel {
    double value{0.0};
    int last_decay_day{-1};
    int last_daily_add_day{-1};
  };

  BoostChannel clinical_boost_;
  BoostChannel clearance_boost_;
};

#endif /* IMMUNESYSTEM_H */
