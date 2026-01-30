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
  int current_day = Model::get_scheduler()->current_time();
  const auto immune = get_effective_clinical_immunity(current_day);

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
  update_extra_boost_decay(current_day);

  // Update new two-channel boosts
  update_boosts_decay_for_day(current_day);
}

// Immunity boost methods (legacy)
void ImmuneSystem::update_extra_boost_decay(int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  if (last_extra_boost_update_day_ < 0) {
    last_extra_boost_update_day_ = current_day;
    return;
  }

  int dt = current_day - last_extra_boost_update_day_;
  if (dt <= 0) return;

  // Legacy single-channel maps to clearance channel in the new config
  const double lambda = std::log(2.0) / cfg.clearance.half_life_days;
  extra_boost_ *= std::exp(-lambda * dt);

  last_extra_boost_update_day_ = current_day;
}

void ImmuneSystem::add_extra_boost(double amount, int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  update_extra_boost_decay(current_day);

  // Legacy single-channel maps to clearance channel cap
  extra_boost_ = std::min(cfg.clearance.max_extra_boost, extra_boost_ + amount);
}

double ImmuneSystem::get_effective_immunity(int current_day) const {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return get_current_value();

  // Note: This is const, so we can't update decay here. Assume it's updated elsewhere.
  double effective = get_current_value() + extra_boost_;
  return std::clamp(effective, 0.0, 1.0);
}

void ImmuneSystem::add_daily_exposure_boost(int current_day) {
  if (current_day <= last_daily_boost_day_) return;

  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  // Legacy mapping: daily exposure affects clearance channel (backwards-compatible)
  add_extra_boost(cfg.clearance.boost_per_exposure_day, current_day);

  last_daily_boost_day_ = current_day;
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

double ImmuneSystem::get_effective_clinical_immunity(int current_day) const {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return get_current_value();

  double effective = get_current_value() + clinical_boost_.value;
  return std::clamp(effective, 0.0, 1.0);
}

double ImmuneSystem::get_effective_clearance_immunity(int current_day) const {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return get_current_value();

  double effective = get_current_value() + clearance_boost_.value;
  return std::clamp(effective, 0.0, 1.0);
}

