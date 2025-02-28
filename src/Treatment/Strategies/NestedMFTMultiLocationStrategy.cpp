#include <sstream>
#include "NestedMFTMultiLocationStrategy.h"
#include "Simulation/Model.h"
#include "Configuration/Config.h"
#include "Utils/Random.h"
#include "Population/Person/Person.h"
#include "Core/Scheduler/Scheduler.h"
#include "Treatment/Therapies/Therapy.h"


NestedMFTMultiLocationStrategy::NestedMFTMultiLocationStrategy() : IStrategy(
    "NestedMFTMultiLocationStrategy", NestedMFTMultiLocation
) { }

NestedMFTMultiLocationStrategy::~NestedMFTMultiLocationStrategy() = default;

void NestedMFTMultiLocationStrategy::add_strategy(IStrategy* strategy) {
  strategy_list.push_back(strategy);
}

void NestedMFTMultiLocationStrategy::add_therapy(Therapy* therapy) { }

Therapy* NestedMFTMultiLocationStrategy::get_therapy(Person* person) {
  const auto loc = person->get_location();
  const auto p = Model::get_random()->random_flat(0.0, 1.0);

  double sum = 0;
  for (auto i = 0; i < distribution[loc].size(); i++) {
    sum += distribution[loc][i];
    if (p <= sum) {
      return strategy_list[i]->get_therapy(person);
    }
  }
  return strategy_list[strategy_list.size() - 1]->get_therapy(person);
}

std::string NestedMFTMultiLocationStrategy::to_string() const {
  std::stringstream sstm;
  sstm << id << "-" << name;
  // for (auto i : distribution[0]) {
  //   sstm << i << ",";
  // }
  // sstm << std::endl;
  //
  // for (auto i : start_distribution[0]) {
  //   sstm << i << ",";
  // }
  // sstm << std::endl;
  return sstm.str();
}

void NestedMFTMultiLocationStrategy::update_end_of_time_step() {
  // update each strategy in the nest
  for (auto& strategy : strategy_list) {
    strategy->update_end_of_time_step();
  }
}

void NestedMFTMultiLocationStrategy::adjust_distribution(const int& time) {
  if (peak_after == -1) {
    // inflation every year
    for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
      const auto d_act = distribution[loc][0] * (1 + Model::get_config()->get_epidemiological_parameters().get_inflation_factor() / 12);
      distribution[loc][0] = d_act;
      const auto other_d = (1 - d_act) / (distribution[loc].size() - 1);
      for (auto i = 1; i < distribution[loc].size(); i++) {
        distribution[loc][i] = other_d;
      }
    }
  } else {
    // increasing linearly
    if (time <= starting_time + peak_after) {
      if (distribution[0][0] < 1) {
        for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
          for (auto i = 0; i < distribution[loc].size(); i++) {

            auto dist = peak_after == 0 ? peak_distribution[loc][i] :
                        (peak_distribution[loc][i] - start_distribution[loc][i]) * (time - starting_time) /
                        peak_after + start_distribution[loc][i];
            dist = dist > peak_distribution[loc][i] ? peak_distribution[loc][i] : dist;
            distribution[loc][i] = dist;
          }
        }
      }
    }
  } //    std::cout << to_string() << std::endl;
}

void NestedMFTMultiLocationStrategy::adjust_started_time_point(const int& current_time) {
  starting_time = current_time;
  // update each strategy in the nest
  for (auto* strategy : strategy_list) {
    strategy->adjust_started_time_point(current_time);
  }
}

void NestedMFTMultiLocationStrategy::monthly_update() {
  adjust_distribution(Model::get_scheduler()->current_time());

  for (auto* strategy : strategy_list) {
    strategy->monthly_update();
  }

  // for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
  //   std::cout << distribution[loc] << std::endl;
  // }

}
