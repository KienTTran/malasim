#include <atomic>
#include <algorithm>

#include "ImmuneSystem.h"

#include <cmath>
#include <memory>

#include "Configuration//Config.h"
#include "ImmuneComponent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"

ImmuneSystem::ImmuneSystem(Person* person) : person_(person), immune_component_(nullptr) {}

ImmuneSystem::~ImmuneSystem() { person_ = nullptr; }

ImmuneComponent* ImmuneSystem::immune_component() const { return immune_component_.get(); }

void ImmuneSystem::set_immune_component(std::unique_ptr<ImmuneComponent> value) {
  if (value == nullptr) {
    spdlog::error("ImmuneSystem::set_immune_component: value is nullptr");
    throw std::invalid_argument("ImmuneSystem::set_immune_component: value is nullptr");
  }
  value->set_immune_system(this);
  immune_component_ = std::move(value);
}

void ImmuneSystem::draw_random_immune() { immune_component_->draw_random_immune(); }

double ImmuneSystem::get_latest_immune_value() const { return immune_component_->latest_value(); }

void ImmuneSystem::set_latest_immune_value(double value) {
  immune_component_->set_latest_value(value);
}

double ImmuneSystem::get_current_value() const { return immune_component_->get_current_value(); }

double ImmuneSystem::get_parasite_size_after_t_days(const int &duration,
                                                    const double &original_size,
                                                    const double &fitness) const {
  const auto last_immune_level = get_latest_immune_value();
  const auto temp =
      (Model::get_config()->get_immune_system_parameters().c_max * (1 - last_immune_level))
      + (Model::get_config()->get_immune_system_parameters().c_min * last_immune_level);
  // std::cout << "day: " << Model::get_scheduler()->current_time() << "\tc_max: " <<
  // Model::CONFIG->immune_system_information().c_max << "\tc_min: " <<
  // Model::CONFIG->immune_system_information().c_min << "\tlast_immune_level: " <<
  // last_immune_level << "\ttemp: " << temp << std::endl;
  //  std::cout << "Day: " << Model::get_scheduler()->current_time() << "\tImmune: old density: " <<
  //  originalSize << "\t duration: " << duration << "\tfitness: "
  //  << fitness << "\tlast immune level: " << last_immune_level << "\ttemp: " << temp;
  //  std::cout << "\tnew density: " << value << std::endl;
  const auto value = original_size + (duration * (log10(temp) + log10(fitness)));
  return value;
}

double ImmuneSystem::get_clinical_progression_probability() const {
  const auto immune = get_effective_clinical_immunity();

  const auto isf = Model::get_config()->get_immune_system_parameters();

  const auto p_clinical =
      isf.max_clinical_probability
      / (1 + pow((immune / isf.midpoint), isf.immune_effect_on_progression_to_clinical));

  return p_clinical;
}

void ImmuneSystem::update() {
  immune_component_->update();

  // Update extra boost decay (legacy)
  int current_day = Model::get_scheduler()->current_time();

  // Update new two-channel boosts
  update_boosts_decay_for_day(current_day);
}

double ImmuneSystem::get_clinical_immunity_only() const {
  return clinical_boost_.value;
}
double ImmuneSystem::get_clearance_immunity_only() const {
  return clearance_boost_.value;
}

double ImmuneSystem::get_effective_immunity() const {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return get_current_value();

  // Note: This is const, so we can't update decay here. Assume it's updated elsewhere.
  double effective = get_current_value() + clinical_boost_.value + clearance_boost_.value;
  return std::clamp(effective, 0.0, 1.0);
}


double ImmuneSystem::get_effective_clinical_immunity() const {
  double base = get_current_value();
  double eff  = base + clinical_boost_.value;
  return std::clamp(eff, 0.0, 1.0);
}

double ImmuneSystem::get_effective_clearance_immunity() const {
  double base = get_current_value();
  double eff  = base + clearance_boost_.value;
  return std::clamp(eff, 0.0, 1.0);
}


// New two-channel boost helpers
static double safe_decay(double value, double half_life_days, int dt) {
  if (dt <= 0) return value;
  if (half_life_days <= 0) return value; // treat non-positive half-life as no decay
  const double lambda = std::log(2.0) / half_life_days;
  return value * std::exp(-lambda * dt);
}

void ImmuneSystem::update_boosts_decay_for_day(int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  if (current_day <= clinical_boost_.last_decay_day) return;
  // Clinical channel decay
  if (clinical_boost_.last_decay_day < 0) {
    clinical_boost_.last_decay_day = current_day;
  } else {
    int dt = current_day - clinical_boost_.last_decay_day;
    if (dt > 0) {
      clinical_boost_.value = safe_decay(clinical_boost_.value, cfg.clinical.half_life_days, dt);
      clinical_boost_.last_decay_day = current_day;
    }
  }

  // Clearance channel decay
  if (clearance_boost_.last_decay_day < 0) {
    clearance_boost_.last_decay_day = current_day;
  } else {
    int dt = current_day - clearance_boost_.last_decay_day;
    if (dt > 0) {
      clearance_boost_.value = safe_decay(clearance_boost_.value, cfg.clearance.half_life_days, dt);
      clearance_boost_.last_decay_day = current_day;
    }
  }
}

void ImmuneSystem::add_clinical_event_boost(double amount, int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  update_boosts_decay_for_day(current_day);

  // Apply cap
  clinical_boost_.value = std::min(cfg.clinical.max_extra_boost, clinical_boost_.value + amount);
}

void ImmuneSystem::add_clearance_event_boost(double amount, int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  update_boosts_decay_for_day(current_day);

  // Apply cap
  clearance_boost_.value = std::min(cfg.clearance.max_extra_boost, clearance_boost_.value + amount);
}

void ImmuneSystem::add_daily_clearance_exposure_boost(int current_day, double amount_multiplier) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;
  if (current_day <= clearance_boost_.last_daily_add_day) return;

  update_boosts_decay_for_day(current_day);

  double base_amount = cfg.clearance.boost_per_exposure_day;
  double amount = base_amount * amount_multiplier;
  clearance_boost_.value = std::min(cfg.clearance.max_extra_boost, clearance_boost_.value + amount);
  clearance_boost_.last_daily_add_day = current_day;
}

void ImmuneSystem::add_daily_clinical_exposure_boost(int current_day, double amount_multiplier) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;
  if (current_day <= clinical_boost_.last_daily_add_day) return;

  update_boosts_decay_for_day(current_day);

  double base_amount = cfg.clinical.boost_per_exposure_day;
  double amount = base_amount * amount_multiplier;
  clinical_boost_.value = std::min(cfg.clinical.max_extra_boost, clinical_boost_.value + amount);
  clinical_boost_.last_daily_add_day = current_day;
}

static std::atomic<int> g_daily_clinical_boosts_applied{0};
static std::atomic<int> g_daily_clearance_boosts_applied{0};

void ImmuneSystem::reset_daily_learning_counters() {
  g_daily_clinical_boosts_applied.store(0, std::memory_order_relaxed);
  g_daily_clearance_boosts_applied.store(0, std::memory_order_relaxed);
}

int ImmuneSystem::daily_clinical_boosts_applied_today() { return g_daily_clinical_boosts_applied.load(std::memory_order_relaxed); }

int ImmuneSystem::daily_clearance_boosts_applied_today() { return g_daily_clearance_boosts_applied.load(std::memory_order_relaxed); }

void ImmuneSystem::apply_daily_immunity_learning(Person& p, int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  // IMPORTANT: p.update_blood_streaks() must have been called already today.

  double mult = 1.0;
  if (p.has_effective_drug_in_blood() || p.has_any_drugs()) {
    mult = cfg.drug_exposure_multiplier; // reduce, don't disable
  }

  // Clinical learns from any blood-day streak
  if (p.blood_days_streak() >= cfg.clinical.exposure_gate_days) {
    add_daily_clinical_exposure_boost(current_day, mult);
    g_daily_clinical_boosts_applied.fetch_add(1, std::memory_order_relaxed);
  }

  // Clearance learns only from *continuous asymptomatic carriage*
  if (p.asym_blood_days_streak() >= cfg.clearance.exposure_gate_days) {
    add_daily_clearance_exposure_boost(current_day, mult);
    g_daily_clearance_boosts_applied.fetch_add(1, std::memory_order_relaxed);
  }
}


