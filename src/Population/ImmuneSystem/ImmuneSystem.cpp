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
  const auto immune = get_effective_immune_value();

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
  update_treatment_memory();
}

void ImmuneSystem::update_treatment_memory() const {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_treatment_immunity();
  if (!cfg.enable) return;

  int today = Model::get_scheduler()->current_time();
  if (last_memory_update_day_ < 0) {
    last_memory_update_day_ = today;
    return;
  }

  int dt = today - last_memory_update_day_;
  if (dt <= 0) return;

  const double lambda = std::log(2.0) / cfg.half_life_days;
  treatment_memory_I_ *= std::exp(-lambda * dt);
  last_memory_update_day_ = today;
}

void ImmuneSystem::on_treated_clinical_episode() {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_treatment_immunity();
  if (!cfg.enable) return;

  update_treatment_memory();
  treatment_memory_I_ = std::min(cfg.max_extra_boost, treatment_memory_I_ + cfg.boost_on_treatment);
}

void ImmuneSystem::on_untreated_clinical_episode() {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_treatment_immunity();
  if (!cfg.enable) return;

  update_treatment_memory();
  treatment_memory_I_ = std::min(cfg.max_extra_boost, treatment_memory_I_ + cfg.boost_on_untreated_clinical);
}

double ImmuneSystem::get_effective_immune_value() const {
  const auto& cfg = Model::get_config()->get_immune_system_parameters().get_treatment_immunity();
  if (!cfg.enable) return get_current_value();

  update_treatment_memory();
  return std::clamp(get_current_value() + treatment_memory_I_, 0.0, 1.0);
}
