#include "Mosquito.h"

#include <iostream>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Population/Population.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Simulation/Model.h"
#include "Utils/Random.h"
#include "Utils/TypeDef.h"

Mosquito::Mosquito() {
}

void Mosquito::initialize(Config* config) {
  genotypes_table.clear();

  genotypes_table = std::vector<std::vector<std::vector<Genotype*>>>(
      config->number_of_tracking_days(),
      std::vector<std::vector<Genotype*>>(config->number_of_locations(),
                                          std::vector<Genotype*>(100, nullptr)));

  auto &location_db = config->location_db();
  for (auto loc_index = 0; loc_index < location_db.size(); ++loc_index) {
    if (Model::get_population()->all_alive_persons_by_location()[loc_index].empty()) continue;
    for (auto day = 0; day < config->number_of_tracking_days(); ++day) {
      genotypes_table[day][loc_index] =
          std::vector<Genotype*>(location_db[loc_index].mosquito_size, nullptr);
    }
  }
}

void Mosquito::infect_new_cohort_in_PRMC(Config* config, utils::Random* random,
                                         Population* population, const int &tracking_index) {
  auto &location_db = config->location_db();
  // for each location fill prmc at tracking_index row with sampling genotypes
  for (int loc = 0; loc < config->number_of_locations(); loc++) {
    if (!Model::get_population()->all_alive_persons_by_location()[loc].empty()) {
      // spdlog::info("tracking_index {} Location {} has {} alive persons", tracking_index,loc,
      //   Model::get_population()->all_alive_persons_by_location[loc].size());
    } else {
      // spdlog::info("infect_new_cohort_in_PRMC tracking_index {} Location {} has no alive
      // persons", tracking_index,loc);
      continue;
    }
    spdlog::trace("Day {} ifr = {}", Model::get_scheduler()->current_time(),
                  location_db[loc].mosquito_ifr);
    // if there is no parasites in location
    if (population->current_force_of_infection_by_location()[loc] <= 0) {
      for (int i = 0; i < location_db[loc].mosquito_size; ++i) {
        genotypes_table[tracking_index][loc][i] = nullptr;
      }
      return;
    }
    // multinomial sampling of people based on their relative infectivity (summing across all clones inside that person)
    auto first_sampling = random->roulette_sampling<Person>(location_db[loc].mosquito_size,
                                                            population->individual_foi_by_location()[loc],
                                                            population->all_alive_persons_by_location()[loc], false,
                                                            population->current_force_of_infection_by_location()[loc]);

    std::vector<unsigned int> interrupted_feeding_indices = build_interrupted_feeding_indices(
        random, location_db[loc].mosquito_ifr, location_db[loc].mosquito_size);

    // uniform sampling in all person
    auto second_sampling = random->roulette_sampling<Person>(location_db[loc].mosquito_size,
                                                             population->individual_relative_biting_by_location()[loc],
                                                             population->all_alive_persons_by_location()[loc], true);

    // recombination
    // *p1 , *p2, bool is_interrupted  ===> *genotype
    std::vector<Genotype *> sampled_genotypes;
    std::vector<double> relative_infectivity_each_pp;

    for (int if_index = 0; if_index < interrupted_feeding_indices.size(); ++if_index) {
      // clear() is used to avoid memory reallocation
      sampled_genotypes.clear();
      relative_infectivity_each_pp.clear();

        /* There are 4 cases:
         * 1. WH=1,IF=1: recombination between two persons
         *    - Get 2 sample genotypes from 2 person
         *    - Select 2 genotypes from 2 sampled genotypes
         *    - Recombine 2 selected genotypes
         * 2. WH=1,IF=0: recombination within one person
         *    - Get 1 sample genotypes from 1 person
         *    - Select 2 genotypes from 1 sampled genotypes
         *    - Recombine 2 selected genotypes
         * 3. WH=0,IF=1: recombination between two persons
         *    - Get 1 genotype from each of 2 person
         *    - Recombine 2 selected genotypes
         * 4. WH=0,IF=0: recombination inside 1 persons
         *   - Get 1 genotype from 1 person
         *   - Recombine 1 selected genotypes (nothing happen)
        */
      if (config->get_mosquito_parameters().get_within_host_induced_free_recombination()) {
        // get all infectious parasites from first person
        get_genotypes_profile_from_person(first_sampling[if_index], sampled_genotypes, relative_infectivity_each_pp);

        if (sampled_genotypes.empty()) {
          spdlog::error("First person has no infectious parasites, log10_total_infectious_denstiy = {}",
                        first_sampling[if_index]->get_all_clonal_parasite_populations()->log10_total_infectious_density());
        }

        if (interrupted_feeding_indices[if_index]) {
          // if second person is the same as first person, re-select second person until it is different from first.
          // this is to avoid recombination between the same person because in this case the interrupted feeding is true,
          // this is worst case scenario
          auto temp_if = if_index;
          int same_person_counter = 0;
          while (second_sampling[temp_if] == first_sampling[if_index]) {
            temp_if = random->random_uniform(second_sampling.size());
            if (second_sampling[temp_if] == first_sampling[if_index]) {
              same_person_counter++;
            }
            if (same_person_counter > 10) {
              spdlog::trace("second sampling is the same as first sampling, because there is 1 person and IFR is non-zero");
              break;
            }
          }
          // interrupted feeding occurs
          get_genotypes_profile_from_person(second_sampling[temp_if], sampled_genotypes, relative_infectivity_each_pp);
          //Count interrupted feeding events with within host induced recombination on
          Model::get_mdc()->mosquito_recombination_events_count()[loc][0]++;
        }

        if (sampled_genotypes.empty()) {
          spdlog::error("Sampled genotypes should not be empty");
        }
      } else {
        sampled_genotypes.clear();
        relative_infectivity_each_pp.clear();
        get_genotypes_profile_from_person(first_sampling[if_index], sampled_genotypes, relative_infectivity_each_pp);
        // get exactly 1 infectious parasite from first person
        auto first_genotype =
            random->roulette_sampling_tuple<Genotype>(1, relative_infectivity_each_pp, sampled_genotypes, false)[0];

        std::tuple<Genotype *, double> second_genotype = std::make_tuple(nullptr, 0.0);

        if (interrupted_feeding_indices[if_index]) {
          // if second person is the same as first person, re-select second person until it is different from first.
          // this is to avoid recombination between the same person because in this case the interrupted feeding is true,
          // this is worst case scenario
          auto temp_if = if_index;
          while (second_sampling[temp_if] == first_sampling[if_index]) {
            temp_if = random->random_uniform(second_sampling.size());
          }
          sampled_genotypes.clear();
          relative_infectivity_each_pp.clear();
          get_genotypes_profile_from_person(second_sampling[temp_if], sampled_genotypes, relative_infectivity_each_pp);

          if (sampled_genotypes.size() > 0) {
            second_genotype = random->roulette_sampling_tuple<Genotype>(1, relative_infectivity_each_pp,
                                                                        sampled_genotypes, false)[0];
          }
          //Count interrupted feeding events with within host induced recombination off
          Model::get_mdc()->mosquito_recombination_events_count()[loc][0]++;
        }

        sampled_genotypes.clear();
        relative_infectivity_each_pp.clear();
        sampled_genotypes.push_back(std::get<0>(first_genotype));
        relative_infectivity_each_pp.push_back(std::get<1>(first_genotype));

        if (std::get<0>(second_genotype) != nullptr) {
          sampled_genotypes.push_back(std::get<0>(second_genotype));
          relative_infectivity_each_pp.push_back(std::get<1>(second_genotype));
        }
      }

      /* The sampling 2 genotypes here are WITH replacement (see roulette sampling code)
       * 1. We select two people with different g(density) and sample genotypes from them,
       * 2. We select two genotypes based on their relative infectivity so there is a case
       * that we select the same genotype twice.
       * */
      auto parent_genotypes = random->roulette_sampling<Genotype>(2, relative_infectivity_each_pp, sampled_genotypes, false);

      Genotype *sampled_genotype =
          (parent_genotypes[0]->aa_sequence == parent_genotypes[1]->aa_sequence)
              ? parent_genotypes[0]
              : Genotype::free_recombine(config, random, parent_genotypes[0], parent_genotypes[1]);

      // Genotype *fix_genotype = Model::get_genotype_db()->at(0);
      // genotypes_table[tracking_index][loc][if_index] = fix_genotype;
      genotypes_table[tracking_index][loc][if_index] = sampled_genotype;

      if (config->get_mosquito_parameters().get_record_recombination_events()) {
        //Count DHA-PPQ(8) ASAQ(7) AL(6)
        //Count if male genotype resists to one drug and female genotype resists to another drug only, right now work on double and triple resistant only
        //when genotype ec50_power_n == min_ec50, it is sensitive to that drug
        if (Model::get_scheduler()->current_time() >= Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period())
        {
          /*
           * Print our recombination for counting later
           * */
          auto resistant_tracker_info = std::make_tuple(Model::get_scheduler()->current_time(),
            parent_genotypes[0]->genotype_id(), parent_genotypes[1]->genotype_id(), sampled_genotype->genotype_id());
          //            if (std::find(Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc].begin(),
          //                          Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc].end(), resistant_tracker_info)
          //                == Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc].end()){
          //                Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc].push_back(resistant_tracker_info);
          //            }
          Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[loc].push_back(resistant_tracker_info);
        }
      }
      //Count number of bites
      Model::get_mdc()->mosquito_recombination_events_count()[loc][1]++;
    }
  }
}

std::vector<unsigned int> Mosquito::build_interrupted_feeding_indices(
    utils::Random* random, const double &interrupted_feeding_rate, const int &prmc_size) {
  int number_of_interrupted_feeding = random->random_poisson(interrupted_feeding_rate * prmc_size);

  std::vector<unsigned int> all_interrupted_feeding(number_of_interrupted_feeding, 1);
  all_interrupted_feeding.resize(prmc_size, 0);

  // spdlog::info("Number of interrupted feeding: {}", number_of_interrupted_feeding);
  // spdlog::info("Interrupted feeding rate: {}", interrupted_feeding_rate);
  // spdlog::info("PRMC size: {}", prmc_size);
  random->shuffle(all_interrupted_feeding);
  return all_interrupted_feeding;
}

int Mosquito::random_genotype(int location, int tracking_index) {
  // Get number of genotypes in genotypes_table
  auto &location_db = Model::get_config()->location_db();
  auto genotype_index = Model::get_random()->random_uniform<int>(0, location_db[location].mosquito_size);
  if (genotypes_table[tracking_index][location][genotype_index] == nullptr) return -1;
  return genotypes_table[tracking_index][location][genotype_index]->genotype_id();
}

void Mosquito::get_genotypes_profile_from_person(
    Person* person, std::vector<Genotype*> &sampling_genotypes,
    std::vector<double> &relative_infectivity_each_pp) {
  for (auto &pp : *person->get_all_clonal_parasite_populations()) {
    // Select parasites based on gametocyte density
    auto clonal_foi = pp->gametocyte_level()
                      * Person::relative_infectivity(pp->last_update_log10_parasite_density());
    if (clonal_foi > 0) {
      relative_infectivity_each_pp.push_back(clonal_foi);
      sampling_genotypes.push_back(pp->genotype());
    }
  }
}

/*
 * For DxG
 */

std::string Mosquito::get_old_genotype_string(std::string new_genotype) {
  std::vector<std::string> pattern_chr = StringHelpers::split(new_genotype, '|');
  std::string old_chr_7 = pattern_chr[6].substr(0, 7);
  const std::string &old_chr_5 = pattern_chr[4];
  std::string old_chr_13 = pattern_chr[12].substr(0, 13);
  std::string old_chr_14 = pattern_chr[13].substr(0, 1);
  std::string old_chr_x = pattern_chr[6].substr(6, 1);
  return old_chr_5 + "|" + old_chr_7 + "|" + old_chr_13 + "|" + old_chr_14;
}

std::string Mosquito::get_old_genotype_string2(std::string new_genotype) {
  std::vector<std::string> pattern_chr = StringHelpers::split(new_genotype, '|');
  std::string old_chr_7 = pattern_chr[6].substr(0, 7);
  const std::string &old_chr_5 = pattern_chr[4];
  std::string old_chr_13 = pattern_chr[12].substr(0, 13);
  std::string old_chr_14 = pattern_chr[13].substr(0, 1);
  std::string old_chr_x = pattern_chr[6].substr(6, 1);
  return old_chr_7[0] + old_chr_5 + old_chr_13[10] + old_chr_14;
}

