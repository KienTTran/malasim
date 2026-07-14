#define NOMINMAX

#include "SMCEvent.h"

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Events/ReceiveMDATherapyEvent.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Utils/Helpers/StringHelpers.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "Utils/Random.h"
#include "date/date.h"

SMCEvent::SMCEvent(const int& at_time) {
    set_time(at_time);
    days_to_complete_all_treatments = 14;
    fraction_population_targeted = std::vector<double>();
}

void SMCEvent::do_execute() {
  spdlog::info("{}: executing Single Round SMC", Model::get_scheduler()->get_current_date_string());

  for(auto &district : districts){
    if (district < 1 || district > Model::get_spatial_data()->get_boundary("district")->max_unit_id) {
      spdlog::error("District ID {} is out of valid range [1, {}]", district,
                    Model::get_spatial_data()->get_boundary("district")->max_unit_id);

      return;
    }

    
    
    
    // Find the corresponding index in the district
    auto it = std::find(districts.begin(), districts.end(), district);
    std::size_t district_index = std::distance(districts.begin(), it);
    double fraction = fraction_population_targeted[district_index];
    
    // Get all pixels that belong to the district
    auto locations = Model::get_spatial_data()->get_locations_in_unit("district", district);


    auto* pi_lsa = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

    std::vector<Person*> eligible_persons;

    for (auto hs = 0; hs < Person::DEAD; hs++) {
      for (std::size_t ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        for (auto loc : locations) {
          for (auto* p : pi_lsa->vPerson()[loc][hs][ac]) {

            double age_in_years = p->age_in_floating(Model::get_scheduler()->current_time());
            double min_age_years = age_range[0] / 12.0;
            double max_age_years = age_range[1] / 12.0;

            if (age_in_years >= min_age_years && age_in_years < max_age_years) {
                 eligible_persons.push_back(p);
            }
          
          }
        }
      }
    }

    const std::size_t total_eligible = eligible_persons.size();
    unsigned int num_targeted = Model::get_random()->random_poisson(fraction * total_eligible);

    if (fraction >= 1.0) {
      num_targeted = total_eligible;
    } 
    else {
      num_targeted = Model::get_random()->random_poisson(fraction * total_eligible);
      if (num_targeted > total_eligible) {
        num_targeted = total_eligible;
      }
    }

    if (!eligible_persons.empty()) {
      Model::get_random()->shuffle(eligible_persons);
    }

   for (std::size_t p_i = 0; p_i < num_targeted; ++p_i) {
      auto* person = eligible_persons[p_i];

      const auto prob = Model::get_random()->random_flat(0.0, 1.0);
      if (prob <= person->prob_present_at_smc()) {
        auto* therapy = Model::get_therapy_db()[Model::get_config()->get_strategy_parameters().get_smc().get_smc_therapy_id()].get();

        int days_to_receive_smc = Model::get_random()->random_uniform(days_to_complete_all_treatments) + 1;
    
        person->schedule_receive_smc_therapy_event(therapy, days_to_receive_smc);

        
      }
    }



    







  }

  // for all location
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    // step 1: get number of individuals for SMC
    auto* pi_lsa = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
    std::vector<Person*> all_persons_in_location;
    for (auto hs = 0; hs < Person::DEAD; hs++) {
      for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        for (auto* person : pi_lsa->vPerson()[loc][hs][ac]) {
          all_persons_in_location.push_back(person);
        }
      }
    }

    const auto number_indidividuals_in_location = all_persons_in_location.size();
    auto number_of_individuals_will_receive_smc = Model::get_random()->random_poisson(
        fraction_population_targeted[loc] * number_indidividuals_in_location);

    number_of_individuals_will_receive_smc =
        number_of_individuals_will_receive_smc > number_indidividuals_in_location
            ? number_indidividuals_in_location
            : number_of_individuals_will_receive_smc;
    // shuffle app_persons_in_location index for sampling without replacement
    Model::get_random()->shuffle(all_persons_in_location);

    for (auto p_i = 0; p_i < number_of_individuals_will_receive_smc; p_i++) {
      auto* person = all_persons_in_location[p_i];
      // step 2: determine whether person will receive treatment
      const auto prob = Model::get_random()->random_flat(0.0, 1.0);
      if (prob < person->prob_present_at_smc()) {
        // receive SMC
        auto* therapy =
            Model::get_therapy_db()
                [Model::get_config()->get_strategy_parameters().get_smc().get_smc_therapy_id()]
                    .get();
        // schedule received therapy in within days_to_complete_all_treatments
        int days_to_receive_smc_therapy =
            Model::get_random()->random_uniform(days_to_complete_all_treatments) + 1;

        person->schedule_receive_smc_therapy_event(therapy, days_to_receive_smc_therapy);
      }
    }
  }
}
