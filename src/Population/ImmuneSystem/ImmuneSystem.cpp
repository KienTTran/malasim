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
  const auto value = original_size + (duration * (log10(temp) + log10(fitness)));
  //  std::cout << "\tnew density: " << value << std::endl;
  return value;
}

double ImmuneSystem::get_clinical_progression_probability() const {
  int current_day = Model::get_scheduler()->current_time();
  const auto immune = get_effective_immunity(current_day);

  const auto isf = Model::get_config()->get_immune_system_parameters();

  //    double PClinical = (isf.min_clinical_probability - isf.max_clinical_probability) *
  //    pow(immune, isf.immune_effect_on_progression_to_clinical) + isf.max_clinical_probability;

  //    const double p_m = 0.99;

  const auto p_clinical =
      isf.max_clinical_probability
      / (1 + pow((immune / isf.midpoint), isf.immune_effect_on_progression_to_clinical));

  // spdlog::info(
  //     "ImmuneSystem::get_clinical_progression_probability: immune: {}, PClinical: {}, max
  //     clinical " "probability: {}, immune effect on progression to clinical: {}", immune,
  //     p_clinical, isf.max_clinical_probability, isf.immune_effect_on_progression_to_clinical);
  //    std::cout << immune << "\t" << PClinical<< std::endl;
  return p_clinical;
}

void ImmuneSystem::update() {
  immune_component_->update();

  // Update extra boost decay
  int current_day = Model::get_scheduler()->current_time();
  update_extra_boost_decay(current_day);
}

// Immunity boost methods
void ImmuneSystem::update_extra_boost_decay(int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  if (last_extra_boost_update_day_ < 0) {
    last_extra_boost_update_day_ = current_day;
    return;
  }

  int dt = current_day - last_extra_boost_update_day_;
  if (dt <= 0) return;

  const double lambda = std::log(2.0) / cfg.half_life_days;
  extra_boost_ *= std::exp(-lambda * dt);

  last_extra_boost_update_day_ = current_day;
}

void ImmuneSystem::add_extra_boost(double amount, int current_day) {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_immunity_boost();
  if (!cfg.enable) return;

  update_extra_boost_decay(current_day);

  extra_boost_ = std::min(cfg.max_extra_boost, extra_boost_ + amount);
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

  add_extra_boost(cfg.boost_per_exposure_day, current_day);

  last_daily_boost_day_ = current_day;
}
