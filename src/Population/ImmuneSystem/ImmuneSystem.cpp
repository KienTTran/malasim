#include "ImmuneSystem.h"
#include "ImmuneComponent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Configuration//Config.h"
#include <cmath>
#include "Utils/Helpers/ObjectHelpers.h"

ImmuneSystem::ImmuneSystem(Person *p) : person_(p), increase_(false), immune_component_(nullptr) {
  //    immune_components_ = new ImmuneComponentPtrVector();

}

ImmuneSystem::~ImmuneSystem() {

  if (immune_component_!=nullptr) {
    ObjectHelpers::delete_pointer<ImmuneComponent>(immune_component_);
  }
  assert(immune_component_==nullptr);
  person_ = nullptr;
}

ImmuneComponent *ImmuneSystem::immune_component() const {
  return immune_component_;
}

void ImmuneSystem::set_immune_component(ImmuneComponent *value) {
  if (immune_component_!=value) {
    if (immune_component_!=nullptr) {
      ObjectHelpers::delete_pointer<ImmuneComponent>(immune_component_);
    }

    immune_component_ = value;
    immune_component_->set_immune_system(this);
  }
}

void ImmuneSystem::draw_random_immune() {
  immune_component_->draw_random_immune();
}

double ImmuneSystem::get_lastest_immune_value() const {
  return immune_component_->latest_value();
}

void ImmuneSystem::set_latest_immune_value(double value) {
  immune_component_->set_latest_value(value);
}

double ImmuneSystem::get_current_value() const {
  return immune_component_->get_current_value();
}

double ImmuneSystem::get_parasite_size_after_t_days(const int &duration, const double &originalSize,
                                                    const double &fitness) const {

  const auto last_immune_level = get_lastest_immune_value();
  const auto temp = Model::get_config()->get_immune_system_parameters().c_max*(1 - last_immune_level)
  + Model::get_config()->get_immune_system_parameters().c_min*last_immune_level;
// std::cout << "day: " << Model::get_scheduler()->current_time() << "\tc_max: " << Model::CONFIG->immune_system_information().c_max << "\tc_min: " << Model::CONFIG->immune_system_information().c_min << "\tlast_immune_level: " << last_immune_level << "\ttemp: " << temp << std::endl;
//  std::cout << "Day: " << Model::get_scheduler()->current_time() << "\tImmune: old density: " << originalSize << "\t duration: " << duration << "\tfitness: "
//  << fitness << "\tlast immune level: " << last_immune_level << "\ttemp: " << temp;
  const auto value = originalSize + duration*(log10(temp) + log10(fitness));
//  std::cout << "\tnew density: " << value << std::endl;
  return value;

}

const double mid_point = 0.4;

double ImmuneSystem::get_clinical_progression_probability() const {
  const auto immune = get_current_value();

  const auto isf = Model::get_config()->get_immune_system_parameters();

  //    double PClinical = (isf.min_clinical_probability - isf.max_clinical_probability) * pow(immune, isf.immune_effect_on_progression_to_clinical) + isf.max_clinical_probability;

  //    const double p_m = 0.99;


  const auto p_clinical = isf.max_clinical_probability/(1 + pow((immune/mid_point),
                                                                isf.immune_effect_on_progression_to_clinical));

  //    std::cout << immune << "\t" << PClinical<< std::endl;
  return p_clinical;
}

void ImmuneSystem::update() {
  immune_component_->update();
}
