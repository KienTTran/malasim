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

// double ImmuneSystem::get_clinical_progression_probability() const {
//   const auto immune = get_current_value();
//
//   const auto isf = Model::get_config()->get_immune_system_parameters();
//
//   // Check get_percentage_deciding_to_not_seek_treatment
//   const auto& not_seeking_treatment_list = Model::get_config()->get_epidemiological_parameters().get_percentage_deciding_to_not_seek_treatment();
//   for (size_t i = 0; i < not_seeking_treatment_list.size(); ++i) {
//     const auto& entry = not_seeking_treatment_list[i];
//     if (person_->get_age() >= entry.get_age()) {
//       const auto prob = Model::get_random()->random_flat(0.0, 1.0);
//       if (prob < entry.get_percentage()) {
//         // Record the count
//         if (Model::get_mdc()->recording_data()) {
//           Model::get_mdc()->monthly_number_of_not_seeking_treatment_by_location_index()[person_->get_location()][i]++;
//         }
//         return 0.0;  // Don't progress to clinical
//       }
//     }
//   }
//
//   //    double PClinical = (isf.min_clinical_probability - isf.max_clinical_probability) *
//   //    pow(immune, isf.immune_effect_on_progression_to_clinical) + isf.max_clinical_probability;
//
//   //    const double p_m = 0.99;
//
//   const auto p_clinical =
//       isf.max_clinical_probability
//       / (1 + pow((immune / isf.midpoint), isf.immune_effect_on_progression_to_clinical));
//
//   // spdlog::info(
//   //     "ImmuneSystem::get_clinical_progression_probability: immune: {}, PClinical: {}, max
//   //     clinical " "probability: {}, immune effect on progression to clinical: {}", immune,
//   //     p_clinical, isf.max_clinical_probability, isf.immune_effect_on_progression_to_clinical);
//   //    std::cout << immune << "\t" << PClinical<< std::endl;
//   return p_clinical;
// }

double ImmuneSystem::get_clinical_progression_probability() const {
  const auto immune = get_current_value();
  const auto isf = Model::get_config()->get_immune_system_parameters();

  const auto& epi = Model::get_config()->get_epidemiological_parameters();
  const auto& age_cfg = epi.get_age_based_probability_of_seeking_treatment();

  double p_not_seek = 0.0;
  int record_i = 0;

  // If new age-based config is enabled and has ages, use it.
  if (age_cfg.enabled() && !age_cfg.get_ages().empty()) {
    const int age = person_->get_age();
    const auto& ages = age_cfg.get_ages();

    int idx = 0;
    if (age <= ages.front()) {
      idx = 0;
    } else if (age >= ages.back()) {
      idx = static_cast<int>(ages.size()) - 1;
    } else {
      int hi = 1;
      while (hi < static_cast<int>(ages.size()) && age >= ages[hi]) ++hi;
      idx = hi - 1;
    }

    double age_modifier = 1.0;
    if (age_cfg.get_type() == "power") {
      const double base = age_cfg.get_power().get_base();
      const std::string& exponent_source = age_cfg.get_power().get_exponent_source();
      if (exponent_source == "index") {
        age_modifier = std::pow(base, static_cast<double>(idx + 1));
      } else {
        age_modifier = base; // fallback
      }
    }

    p_not_seek = 1.0 - age_modifier;
    if (p_not_seek < 0.0) p_not_seek = 0.0;
    if (p_not_seek > 1.0) p_not_seek = 1.0;
    record_i = idx;
  }
  // Otherwise, fallback to legacy percentage list (interpolated per-age percentages)
  else {
    const auto& list = epi.get_percentage_deciding_to_not_seek_treatment();
    if (!list.empty()) {
      const int age = person_->get_age();
      // Clamp below first point
      if (age <= list.front().get_age()) {
        p_not_seek = list.front().get_percentage();
        record_i = 0;
      }
      // Clamp above last point
      else if (age >= list.back().get_age()) {
        p_not_seek = list.back().get_percentage();
        record_i = static_cast<int>(list.size()) - 1;
      }
      // Interpolate between surrounding points
      else {
        int hi = 1;
        while (hi < static_cast<int>(list.size()) && age > list[hi].get_age()) ++hi;
        int lo = hi - 1;

        const double a0 = list[lo].get_age();
        const double a1 = list[hi].get_age();
        const double t = (age - a0) / (a1 - a0);

        auto lerp = [](double a, double b, double t) { return a + (b - a) * t; };
        p_not_seek = lerp(list[lo].get_percentage(), list[hi].get_percentage(), t);

        // For recording, pick the lower bucket (same semantics as previous code)
        record_i = lo;
      }
    }
  }

  // If configured to not seek treatment for this person, record and return 0
  if (p_not_seek > 0.0) {
    const double rand_val = Model::get_random()->random_flat(0.0, 1.0);
    if (rand_val < p_not_seek) {
      if (Model::get_mdc()->recording_data()) {
        auto &vec = Model::get_mdc()->monthly_number_of_not_seeking_treatment_by_location_index()[person_->get_location()];
        if (!vec.empty()) {
          if (record_i < static_cast<int>(vec.size())) {
            vec[record_i]++;
          } else {
            vec[vec.size() - 1]++;
          }
        }
      }
      return 0.0;
    }
  }

  const auto p_clinical =
      isf.max_clinical_probability
      / (1 + std::pow((immune / isf.midpoint), isf.immune_effect_on_progression_to_clinical));

  return p_clinical;
}


void ImmuneSystem::update() { immune_component_->update(); }
