#include <gtest/gtest.h>

#include <algorithm>

#include "Mosquito/Mosquito.h"
#include "Parasites/Genotype.h"
#include "Population/Population.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Constants.h"
#include "fixtures/TestFileGenerators.h"

class DailyLocationPipelineTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::create_complete_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override { test_fixtures::cleanup_test_files(); }
};

TEST_F(DailyLocationPipelineTest, NewbornIsIncludedInAlignedSamplingState) {
  auto* population = Model::get_population();
  constexpr int location = 0;
  auto* newborn = population->give_1_birth(location);

  population->update_current_foi();

  const auto &alive_people = population->all_alive_persons_by_location()[location];
  EXPECT_NE(std::find(alive_people.begin(), alive_people.end(), newborn), alive_people.end());
  EXPECT_EQ(alive_people.size(), population->individual_foi_by_location()[location].size());
  EXPECT_EQ(alive_people.size(),
            population->individual_relative_biting_by_location()[location].size());
  EXPECT_EQ(alive_people.size(),
            population->individual_relative_moving_by_location()[location].size());
}

TEST_F(DailyLocationPipelineTest, LocationBirthEventAppendsNewbornSamplingEntries) {
  auto* population = Model::get_population();
  constexpr int location = 0;
  population->update_current_foi();
  const auto initial_size = population->all_alive_persons_by_location()[location].size();
  Model::get_config()->get_population_demographic().set_birth_rate(Constants::DAYS_IN_YEAR);

  population->perform_birth_event_at_location(location);

  const auto &alive_people = population->all_alive_persons_by_location()[location];
  EXPECT_GT(alive_people.size(), initial_size);
  EXPECT_EQ(alive_people.size(), population->individual_foi_by_location()[location].size());
  EXPECT_EQ(alive_people.size(),
            population->individual_relative_biting_by_location()[location].size());
  EXPECT_EQ(alive_people.size(),
            population->individual_relative_moving_by_location()[location].size());
}

TEST_F(DailyLocationPipelineTest, DeadPersonIsRemovedFromAlignedSamplingStateBeforeDeletion) {
  auto* population = Model::get_population();
  constexpr int location = 0;
  auto* person = population->give_1_birth(location);
  population->update_current_foi();
  const auto expected_size = population->all_alive_persons_by_location()[location].size() - 1;

  person->set_host_state(Person::DEAD);
  population->clear_dead_people_at_location(location);

  const auto &alive_people = population->all_alive_persons_by_location()[location];
  EXPECT_EQ(alive_people.size(), expected_size);
  EXPECT_EQ(alive_people.size(), population->individual_foi_by_location()[location].size());
  EXPECT_EQ(alive_people.size(),
            population->individual_relative_biting_by_location()[location].size());
  EXPECT_EQ(alive_people.size(),
            population->individual_relative_moving_by_location()[location].size());
}

TEST_F(DailyLocationPipelineTest, ZeroFoiLocationDoesNotPreventLaterCohortClearing) {
  auto* config = Model::get_config();
  ASSERT_GE(config->number_of_locations(), 2);
  auto* population = Model::get_population();
  auto* mosquito = Model::get_mosquito();
  auto* genotype = Model::get_genotype_db()->at(0);
  constexpr int tracking_index = 0;

  population->current_force_of_infection_by_location()[0] = 0.0;
  population->current_force_of_infection_by_location()[1] = 0.0;
  mosquito->genotypes_table[tracking_index][0][0] = genotype;
  mosquito->genotypes_table[tracking_index][1][0] = genotype;

  mosquito->infect_new_cohort_in_prmc(config, Model::get_random(), population, tracking_index);

  EXPECT_EQ(mosquito->genotypes_table[tracking_index][0][0], nullptr);
  EXPECT_EQ(mosquito->genotypes_table[tracking_index][1][0], nullptr);
}

TEST_F(DailyLocationPipelineTest, DailyUpdateLeavesEveryLocationSamplingStateAligned) {
  EXPECT_NO_THROW(Model::get_instance()->daily_update());

  auto* population = Model::get_population();
  for (auto location = 0; location < Model::get_config()->number_of_locations(); ++location) {
    const auto &alive_people = population->all_alive_persons_by_location()[location];
    EXPECT_EQ(alive_people.size(), population->individual_foi_by_location()[location].size());
    EXPECT_EQ(alive_people.size(),
              population->individual_relative_biting_by_location()[location].size());
    EXPECT_EQ(alive_people.size(),
              population->individual_relative_moving_by_location()[location].size());
    for (const auto* person : alive_people) { EXPECT_NE(person->get_host_state(), Person::DEAD); }
  }
}

TEST_F(DailyLocationPipelineTest, RandomGenotypeReturnsStoredCohortGenotype) {
  auto* mosquito = Model::get_mosquito();
  auto* genotype = Model::get_genotype_db()->at(0);
  const auto mosquito_size = Model::get_config()->location_db()[0].mosquito_size;
  ASSERT_GT(mosquito_size, 0);
  for (int mosquito_index = 0; mosquito_index < mosquito_size; ++mosquito_index) {
    mosquito->genotypes_table[0][0][mosquito_index] = genotype;
  }
  EXPECT_EQ(mosquito->random_genotype(0, 0), genotype->genotype_id());
}

TEST_F(DailyLocationPipelineTest, InfectionPipelineSkipsLocationsWithoutRecipients) {
  auto* population = Model::get_population();
  constexpr int location = 0;
  population->force_of_infection_for_n_days_by_location()[0][location] = 1.0;
  population->all_alive_persons_by_location()[location].clear();
  EXPECT_NO_THROW(population->perform_infection_event_at_location(location, 0));
}

TEST_F(DailyLocationPipelineTest, InfectionPipelineSkipsLocationsWithoutBitingWeight) {
  auto* population = Model::get_population();
  constexpr int location = 0;
  population->force_of_infection_for_n_days_by_location()[0][location] = 1.0;
  population->sum_relative_biting_by_location()[location] = 0.0;
  EXPECT_NO_THROW(population->perform_infection_event_at_location(location, 0));
}

TEST_F(DailyLocationPipelineTest, InfectionPipelineSkipsLocationsWithoutMosquitoGenotypes) {
  auto* population = Model::get_population();
  constexpr int location = 0;
  population->force_of_infection_for_n_days_by_location()[0][location] = 1.0;
  population->sum_relative_biting_by_location()[location] = 1.0;
  Model::get_mosquito()->genotypes_table[0][location].clear();
  EXPECT_NO_THROW(population->perform_infection_event_at_location(location, 0));
}

TEST_F(DailyLocationPipelineTest, PopulationSizeSupportsSpecificAgeClass) {
  auto* population = Model::get_population();
  EXPECT_GT(population->size(0, 0), 0U);
}

TEST_F(DailyLocationPipelineTest, ExtractsInfectiousGenotypesFromPerson) {
  auto* person = Model::get_population()->give_1_birth(0);
  ASSERT_NE(person, nullptr);
  auto* parasite = person->add_new_parasite_to_blood(Model::get_genotype_db()->at(0));
  ASSERT_NE(parasite, nullptr);
  parasite->set_gametocyte_level(1.0);
  parasite->set_last_update_log10_parasite_density(4.0);

  std::vector<Genotype*> genotypes;
  std::vector<double> relative_infectivity;
  Mosquito::get_genotypes_profile_from_person(person, genotypes, relative_infectivity);

  ASSERT_EQ(genotypes.size(), 1U);
  EXPECT_EQ(genotypes.front(), Model::get_genotype_db()->at(0));
  ASSERT_EQ(relative_infectivity.size(), 1U);
  EXPECT_GT(relative_infectivity.front(), 0.0);
}

TEST_F(DailyLocationPipelineTest, InfectiousPersonSeedsMosquitoCohort) {
  auto* config = Model::get_config();
  auto* population = Model::get_population();
  auto* mosquito = Model::get_mosquito();
  constexpr int location = 0;
  ASSERT_FALSE(population->all_alive_persons_by_location()[location].empty());
  auto* person = population->all_alive_persons_by_location()[location].front();
  auto* parasite = person->add_new_parasite_to_blood(Model::get_genotype_db()->at(0));
  parasite->set_gametocyte_level(1.0);
  parasite->set_last_update_log10_parasite_density(4.0);

  auto& foi = population->individual_foi_by_location()[location];
  std::fill(foi.begin(), foi.end(), 0.0);
  foi.front() = 1.0;
  population->current_force_of_infection_by_location()[location] = 1.0;
  config->location_db()[location].mosquito_size = 4;
  mosquito->infect_new_cohort_at_location(config, Model::get_random(), population, location, 0);

  ASSERT_GE(mosquito->genotypes_table[0][location].size(), 4U);
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(mosquito->genotypes_table[0][location][i], Model::get_genotype_db()->at(0));
  }

  config->get_mosquito_parameters().set_record_recombination_events(true);
  Model::get_scheduler()->set_current_time(
      config->get_simulation_timeframe().get_start_of_comparison_period());
  mosquito->infect_new_cohort_at_location(config, Model::get_random(), population, location, 0);
  EXPECT_FALSE(Model::get_mdc()->mosquito_recombined_resistant_genotype_tracker[location].empty());

  config->get_mosquito_parameters().set_record_recombination_events(false);
  config->get_mosquito_parameters().set_within_host_induced_free_recombination(false);
  config->location_db()[location].mosquito_size = 2;
  config->location_db()[location].mosquito_ifr = 1.0;
  auto& biting = population->individual_relative_biting_by_location()[location];
  std::fill(biting.begin(), biting.end(), 0.0);
  ASSERT_GE(biting.size(), 2U);
  biting[1] = 1.0;
  mosquito->infect_new_cohort_at_location(config, Model::get_random(), population, location, 0);
  EXPECT_EQ(mosquito->genotypes_table[0][location][0], Model::get_genotype_db()->at(0));
}
