#define NOMINMAX

#include "SMCEvent.h"

#include <algorithm>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Events/ReceiveMDATherapyEvent.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Treatment/Therapies/Therapy.h"
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
  const bool is_sbt = type == "sbt";
  const int resolved_therapy_id =
      therapy_id < 0
          ? Model::get_config()->get_strategy_parameters().get_smc().get_smc_therapy_id()
          : therapy_id;
  if (resolved_therapy_id < 0
      || resolved_therapy_id >= static_cast<int>(Model::get_therapy_db().size())) {
    spdlog::error("{}: {} round skipped: therapy id {} is not in the therapy database",
                  Model::get_scheduler()->get_current_date_string(), is_sbt ? "SBT" : "SMC",
                  resolved_therapy_id);
    return;
  }
  auto* therapy = Model::get_therapy_db()[resolved_therapy_id].get();

  // Log the drug actually used and where it comes from: rounds without their own
  // therapy_id fall back to the global seasonal_malaria_chemoprevention.smc_therapy_id,
  // which also applies to historical (pre-intervention) rounds.
  spdlog::info("{}: executing Single Round {} (therapy {} '{}'{}, age {}-{} months, {} districts)",
               Model::get_scheduler()->get_current_date_string(), is_sbt ? "SBT" : "SMC",
               resolved_therapy_id, therapy->get_name(),
               therapy_id < 0 ? " from global smc_therapy_id" : "", age_range[0], age_range[1],
               districts.size());

  const auto max_district_id = Model::get_spatial_data()->get_boundary("district")->max_unit_id;
  auto* pi_lsa = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  const double min_age_years = age_range[0] / 12.0;
  const double max_age_years = age_range[1] / 12.0;

  for (std::size_t district_index = 0; district_index < districts.size(); ++district_index) {
    const int district = districts[district_index];
    if (district < 1 || district > max_district_id) {
      // Skip only this district (previously `return` aborted the whole round,
      // silently skipping every district listed after it).
      spdlog::error("District ID {} is out of valid range [1, {}]; skipped", district,
                    max_district_id);
      continue;
    }
    if (district_index >= fraction_population_targeted.size()) {
      // Builder broadcasts/validates this; guard against out-of-bounds reads.
      spdlog::error("No fraction_population_targeted for district {} (entry {}); skipped",
                    district, district_index);
      continue;
    }
    const double fraction = fraction_population_targeted[district_index];

    // Get all pixels that belong to the district
    auto locations = Model::get_spatial_data()->get_locations_in_unit("district", district);

    std::vector<Person*> eligible_persons;

    for (auto hs = 0; hs < Person::DEAD; hs++) {
      for (std::size_t ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        for (auto loc : locations) {
          for (auto* p : pi_lsa->vPerson()[loc][hs][ac]) {
            const double age_in_years = p->age_in_floating(Model::get_scheduler()->current_time());
            if (age_in_years >= min_age_years && age_in_years < max_age_years) {
              eligible_persons.push_back(p);
            }
          }
        }
      }
    }

    const std::size_t total_eligible = eligible_persons.size();
    std::size_t num_targeted = 0;
    if (fraction >= 1.0) {
      num_targeted = total_eligible;
    } else if (fraction > 0.0 && total_eligible > 0) {
      // single draw (previously drawn twice, the first result discarded)
      num_targeted = std::min<std::size_t>(
          Model::get_random()->random_poisson(fraction * static_cast<double>(total_eligible)),
          total_eligible);
    }

    if (!eligible_persons.empty()) { Model::get_random()->shuffle(eligible_persons); }

    for (std::size_t p_i = 0; p_i < num_targeted; ++p_i) {
      auto* person = eligible_persons[p_i];

      const auto prob = Model::get_random()->random_flat(0.0, 1.0);
      if (prob <= person->prob_present_at_smc()) {
        const int days_to_receive_smc =
            static_cast<int>(Model::get_random()->random_uniform(
                static_cast<uint64_t>(days_to_complete_all_treatments)))
            + 1;
        person->schedule_receive_smc_therapy_event(therapy, days_to_receive_smc);
      }
    }
  }
}
