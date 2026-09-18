#include <gtest/gtest.h>

#include <fstream>

#include "Configuration/Config.h"
#include "Events/Population/AnnualBetaUpdateEvent.hxx"
#include "Events/Population/AnnualCoverageUpdateEvent.hxx"
#include "Events/Population/ChangeCirculationPercentEvent.hxx"
#include "Events/Population/ChangeInterruptedFeedingRateEvent.h"
#include "Events/Population/ChangeMutationMaskEvent.h"
#include "Events/Population/ChangeMutationProbabilityPerLocusEvent.h"
#include "Events/Population/ChangeTreatmentCoverageEvent.h"
#include "Events/Population/ChangeTreatmentStrategyEvent.h"
#include "Events/Population/ChangeWithinHostInducedFreeRecombinationEvent.h"
#include "Events/Population/DistrictImportationDailyEvent.h"
#include "Events/Population/ImportationEvent.h"
#include "Events/Population/ImportationPeriodicallyEvent.h"
#include "Events/Population/ImportationPeriodicallyRandomEvent.h"
#include "Events/Population/Introduce580YMutantEvent.h"
#include "Events/Population/IntroduceAmodiaquineMutantEvent.h"
#include "Events/Population/IntroduceLumefantrineMutantEvent.h"
#include "Events/Population/IntroduceMutantEvent.hxx"
#include "Events/Population/IntroduceMutantRasterEvent.hxx"
#include "Events/Population/IntroduceParasitesPeriodicallyEventV2.h"
#include "Events/Population/IntroducePlas2CopyParasiteEvent.h"
#include "Events/Population/IntroduceTripleMutantToDPMEvent.h"
#include "Events/Population/ModifyNestedMFTEvent.h"
#include "Events/Population/PopulationEventBuilder.h"
#include "Events/Population/RotateStrategyEvent.h"
#include "Events/Population/SingleRoundMDAEvent.h"
#include "Events/Population/TurnOffMutationEvent.h"
#include "Events/Population/TurnOnMutationEvent.h"
#include "Events/Population/UpdateBetaRasterEvent.hxx"
#include "Parasites/Genotype.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventBuilderTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  Config* config() { return Model::get_config(); }
};

TEST_F(PopulationEventBuilderTest, BuildsMutationAndMdaEventsWithDatesAndValues) {
  const auto mutation = PopulationEventBuilder::build_turn_on_mutation_event(
      YAML::Load("- date: 2024/01/11\n  mutation_probability: 0.25"), config());
  ASSERT_EQ(mutation.size(), 1);
  auto* turn_on = dynamic_cast<TurnOnMutationEvent*>(mutation[0].get());
  ASSERT_NE(turn_on, nullptr);
  EXPECT_EQ(turn_on->get_time(),
            (date::sys_days{date::year{2024} / 1 / 11}
             - date::sys_days{config()->get_simulation_timeframe().get_starting_date()})
                .count());
  EXPECT_DOUBLE_EQ(turn_on->mutation_probability, 0.25);

  const auto turn_off = PopulationEventBuilder::build_turn_off_mutation_event(
      YAML::Load("- date: 2024/01/12"), config());
  ASSERT_EQ(turn_off.size(), 1);
  EXPECT_EQ(turn_off[0]->get_time(),
            (date::sys_days{date::year{2024} / 1 / 12}
             - date::sys_days{config()->get_simulation_timeframe().get_starting_date()})
                .count());

  const auto mda = PopulationEventBuilder::build_single_round_mda_event(
      YAML::Load("- date: 2024/01/02\n  fraction_population_targeted: [0.2]\n  "
                 "days_to_complete_all_treatments: 5"),
      config());
  ASSERT_EQ(mda.size(), 1);
  auto* mda_event = dynamic_cast<SingleRoundMDAEvent*>(mda[0].get());
  ASSERT_NE(mda_event, nullptr);
  EXPECT_EQ(mda_event->get_fraction_population_targeted().size(), config()->number_of_locations());
  EXPECT_DOUBLE_EQ(mda_event->get_fraction_population_targeted()[0], 0.2);
  EXPECT_EQ(mda_event->get_days_to_complete(), 5);
}

TEST_F(PopulationEventBuilderTest, BuildsConfigurationChangeEvents) {
  const auto recombination =
      PopulationEventBuilder::build_change_within_host_induced_free_recombination_events(
          YAML::Load("- date: 2024/01/03\n  value: true"), config());
  ASSERT_EQ(recombination.size(), 1);
  auto* recombination_event =
      dynamic_cast<ChangeWithinHostInducedFreeRecombinationEvent*>(recombination[0].get());
  ASSERT_NE(recombination_event, nullptr);
  EXPECT_TRUE(recombination_event->value);
  EXPECT_EQ(recombination_event->get_time(),
            (date::sys_days{date::year{2024} / 1 / 3}
             - date::sys_days{config()->get_simulation_timeframe().get_starting_date()})
                .count());

  const auto probability =
      PopulationEventBuilder::build_change_mutation_probability_per_locus_events(
          YAML::Load("- date: 2024/01/04\n  mutation_probability_per_locus: 0.5"), config());
  ASSERT_EQ(probability.size(), 1);
  EXPECT_DOUBLE_EQ(
      dynamic_cast<ChangeMutationProbabilityPerLocusEvent*>(probability[0].get())->value, 0.5);

  const auto mask = PopulationEventBuilder::build_change_mutation_mask_events(
      YAML::Load("- date: 2024/01/05\n  mutation_mask: [true, false, true]"), config());
  ASSERT_EQ(mask.size(), 1);
  EXPECT_EQ(dynamic_cast<ChangeMutationMaskEvent*>(mask[0].get())->mask,
            std::vector<bool>({true, false, true}));

  const auto ifr = PopulationEventBuilder::build_change_interrupted_feeding_rate_event(
      YAML::Load("- location: 0\n  date: 2024/01/06\n  interrupted_feeding_rate: 0.3"), config());
  ASSERT_EQ(ifr.size(), 1);
  auto* ifr_event = dynamic_cast<ChangeInterruptedFeedingRateEvent*>(ifr[0].get());
  ASSERT_NE(ifr_event, nullptr);
  EXPECT_EQ(ifr_event->location, 0);
  EXPECT_DOUBLE_EQ(ifr_event->ifr, 0.3);
}

TEST_F(PopulationEventBuilderTest, RejectsInvalidAnnualAndCirculationDefinitions) {
  EXPECT_THROW(PopulationEventBuilder::build_annual_beta_update_event(YAML::Load("[]"), config()),
               std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_annual_coverage_update_event(
          YAML::Load("- date: 2024/01/01\n  rate: 0.1\n- date: 2024/02/01\n  rate: 0.1"), config()),
      std::invalid_argument);
  EXPECT_THROW(PopulationEventBuilder::build_change_circulation_percent_event(
                   YAML::Load("- date: 2024/01/01\n  circulation_percent: 1.1"), config()),
               std::invalid_argument);
  EXPECT_THROW(PopulationEventBuilder::build_change_circulation_percent_event(
                   YAML::Load("- date: 2024/01/01\n  circulation_percent: -0.1"), config()),
               std::invalid_argument);

  EXPECT_THROW(
      PopulationEventBuilder::build_importation_periodically_random_event(
          YAML::Load(
              "- date: 2024/01/02\n  genotype_id: 0\n  count: 1\n  log_parasite_density: 3.0"),
          config()),
      std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_importation_periodically_random_event(
          YAML::Load(
              "- date: 2024/01/01\n  genotype_id: -1\n  count: 1\n  log_parasite_density: 3.0"),
          config()),
      std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_importation_periodically_random_event(
          YAML::Load(
              "- date: 2024/01/01\n  genotype_id: 9999\n  count: 1\n  log_parasite_density: 3.0"),
          config()),
      std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_importation_periodically_random_event(
          YAML::Load(
              "- date: 2024/01/01\n  genotype_id: 0\n  count: 0\n  log_parasite_density: 3.0"),
          config()),
      std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_importation_periodically_random_event(
          YAML::Load(
              "- date: 2024/01/01\n  genotype_id: 0\n  count: 1\n  log_parasite_density: 0.0"),
          config()),
      std::invalid_argument);
  EXPECT_THROW(PopulationEventBuilder::build_update_beta_raster_event(
                   YAML::Load("- date: 2024/01/01\n  beta_raster: missing-beta.asc"), config()),
               std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_rotate_treatment_strategy_event(
          YAML::Load(
              "- date: 2024/01/01\n  years: 0\n  first_strategy_id: 0\n  second_strategy_id: 1"),
          config()),
      std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_rotate_treatment_strategy_event(
          YAML::Load(
              "- date: 2024/01/01\n  years: 1\n  first_strategy_id: -1\n  second_strategy_id: 1"),
          config()),
      std::invalid_argument);
  EXPECT_THROW(
      PopulationEventBuilder::build_rotate_treatment_strategy_event(
          YAML::Load(
              "- date: 2024/01/01\n  years: 1\n  first_strategy_id: 9999\n  second_strategy_id: 1"),
          config()),
      std::invalid_argument);
}

TEST_F(PopulationEventBuilderTest, AcceptsValidStrategyIdsBeyondGenotypeDatabase) {
  const auto strategy_id = static_cast<int>(Model::get_strategy_db().size()) - 1;
  if (strategy_id < 0 || strategy_id < Model::get_genotype_db()->size()) { GTEST_SKIP(); }

  const auto events = PopulationEventBuilder::build_rotate_treatment_strategy_event(
      YAML::Load(std::string("- date: 2024/01/01\n  years: 1\n  first_strategy_id: ")
                 + std::to_string(strategy_id)
                 + "\n  second_strategy_id: " + std::to_string(strategy_id)),
      config());
  ASSERT_EQ(events.size(), 1u);
}

TEST_F(PopulationEventBuilderTest, BuildsAdditionalPopulationEventTypes) {
  const auto annual_beta = PopulationEventBuilder::build_annual_beta_update_event(
      YAML::Load("- date: 2024/01/01\n  rate: 0.1"), config());
  ASSERT_EQ(annual_beta.size(), 1);
  EXPECT_EQ(annual_beta[0]->name(), AnnualBetaUpdateEvent::EVENT_NAME);

  const auto annual_coverage = PopulationEventBuilder::build_annual_coverage_update_event(
      YAML::Load("- date: 2024/01/01\n  rate: 0.1"), config());
  ASSERT_EQ(annual_coverage.size(), 1);
  EXPECT_EQ(annual_coverage[0]->name(), AnnualCoverageUpdateEvent::EVENT_NAME);

  const auto circulation = PopulationEventBuilder::build_change_circulation_percent_event(
      YAML::Load("- date: 2024/01/01\n  circulation_percent: 0.4"), config());
  ASSERT_EQ(circulation.size(), 1);
  EXPECT_EQ(circulation[0]->name(), ChangeCirculationPercentEvent::EVENT_NAME);

  const std::string raster_name = "builder_beta.asc";
  test_fixtures::create_test_raster_file(raster_name, 1, 1, 0.5);
  const auto raster = PopulationEventBuilder::build_update_beta_raster_event(
      YAML::Load("- date: 2024/01/01\n  beta_raster: builder_beta.asc"), config());
  ASSERT_EQ(raster.size(), 1);
  EXPECT_EQ(raster[0]->name(), UpdateBetaRasterEvent::EVENT_NAME);
  std::remove(raster_name.c_str());

  const auto district = PopulationEventBuilder::build_import_district_mutant_daily_events(
      YAML::Load("- district: 1\n  daily_rate: 0.2\n  start_date: 2024/01/01\n  alleles:\n    - "
                 "chromosome: 5\n      locus: 86\n      allele: Y"),
      config());
  ASSERT_EQ(district.size(), 1);
  EXPECT_EQ(district[0]->name(), DistrictImportationDailyEvent::EVENT_NAME);

  const auto random_import = PopulationEventBuilder::build_importation_periodically_random_event(
      YAML::Load("- date: 2024/01/01\n  genotype_id: 0\n  count: 2\n  log_parasite_density: 3.0"),
      config());
  ASSERT_EQ(random_import.size(), 1);
  EXPECT_EQ(random_import[0]->name(), ImportationPeriodicallyRandomEvent::EVENT_NAME);
}

TEST_F(PopulationEventBuilderTest, BuildsMutantEventDefinitionsAndSkipsInvalidLocations) {
  const auto alleles = "    - chromosome: 5\n      locus: 86\n      allele: Y\n";
  const auto amodiaquine =
      PopulationEventBuilder::build_introduce_amodiaquine_mutant_parasite_events(
          YAML::Load(std::string("- location: 0\n  date: 2024/01/01\n  fraction: 0.2\n  alleles:\n")
                     + alleles),
          config());
  ASSERT_EQ(amodiaquine.size(), 1);
  EXPECT_EQ(amodiaquine[0]->name(), IntroduceAmodiaquineMutantEvent::EVENT_NAME);

  const auto lumefantrine =
      PopulationEventBuilder::build_introduce_lumefantrine_mutant_parasite_events(
          YAML::Load(std::string("- location: 0\n  date: 2024/01/01\n  fraction: 0.2\n  alleles:\n")
                     + alleles),
          config());
  ASSERT_EQ(lumefantrine.size(), 1);
  EXPECT_EQ(lumefantrine[0]->name(), IntroduceLumefantrineMutantEvent::EVENT_NAME);

  const auto mutant_580y = PopulationEventBuilder::build_introduce_580Y_mutant_events(
      YAML::Load(std::string("- location: 0\n  date: 2024/01/01\n  fraction: 0.2\n  alleles:\n")
                 + alleles),
      config());
  ASSERT_EQ(mutant_580y.size(), 1);
  EXPECT_EQ(mutant_580y[0]->name(), Introduce580YMutantEvent::EVENT_NAME);

  const auto triple = PopulationEventBuilder::build_introduce_triple_mutant_to_dpm_parasite_events(
      YAML::Load(std::string("- location: 0\n  date: 2024/01/01\n  fraction: 0.2\n  alleles:\n")
                 + alleles),
      config());
  ASSERT_EQ(triple.size(), 1);
  EXPECT_EQ(triple[0]->name(), IntroduceTripleMutantToDPMEvent::EVENT_NAME);

  const auto skipped = PopulationEventBuilder::build_introduce_580Y_mutant_events(
      YAML::Load("- location: 999\n  date: 2024/01/01\n  fraction: 0.2\n  alleles: []"), config());
  EXPECT_TRUE(skipped.empty());
}

TEST_F(PopulationEventBuilderTest, BuildsImportationAndPeriodicDefinitions) {
  const std::string genotype = "||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1";
  const auto importation = PopulationEventBuilder::build_introduce_parasite_events(
      YAML::Load("- location: 0\n  parasite_info:\n    - genotype_aa_sequence: \"" + genotype + "\""
                 + "\n      number_of_cases: 3\n      date: 2024/01/01"),
      config());
  ASSERT_EQ(importation.size(), 1);
  auto* importation_event = dynamic_cast<ImportationEvent*>(importation[0].get());
  ASSERT_NE(importation_event, nullptr);
  EXPECT_EQ(importation_event->get_location(), 0);
  EXPECT_EQ(importation_event->get_number_of_cases(), 3);

  const auto periodic = PopulationEventBuilder::build_introduce_parasites_periodically_events(
      YAML::Load("- location: 0\n  parasite_info:\n    - genotype_aa_sequence: \"" + genotype + "\""
                 + "\n      number_of_cases: 2\n      duration: 30\n      start_date: 2024/01/01"),
      config());
  ASSERT_EQ(periodic.size(), 1);
  auto* periodic_event = dynamic_cast<ImportationPeriodicallyEvent*>(periodic[0].get());
  ASSERT_NE(periodic_event, nullptr);
  EXPECT_EQ(periodic_event->get_location(), 0);
  EXPECT_EQ(periodic_event->get_duration(), 30);
  EXPECT_EQ(periodic_event->get_number_of_cases(), 2);

  const auto plas2 = PopulationEventBuilder::build_introduce_plas2_parasite_events(
      YAML::Load("- location: 0\n  date: 2024/01/01\n  fraction: 0.4"), config());
  ASSERT_EQ(plas2.size(), 1);
  EXPECT_EQ(plas2[0]->name(), IntroducePlas2CopyParasiteEvent::EVENT_NAME);

  const auto nested = PopulationEventBuilder::build_modify_nested_mft_strategy_event(
      YAML::Load("- date: 2024/01/01\n  strategy_id: 0"), config());
  ASSERT_EQ(nested.size(), 1);
  EXPECT_EQ(nested[0]->name(), ModifyNestedMFTEvent::EVENT_NAME);
}

TEST_F(PopulationEventBuilderTest, BuildsThroughPublicDispatcher) {
  YAML::Node node = YAML::Load(
      "name: change_mutation_probability_per_locus\n"
      "info:\n"
      "  - date: 2024/01/08\n"
      "    mutation_probability_per_locus: 0.2");
  const auto events = PopulationEventBuilder::build(node);
  ASSERT_EQ(events.size(), 1);
  EXPECT_EQ(events[0]->name(), ChangeMutationProbabilityPerLocusEvent::EVENT_NAME);
  EXPECT_EQ(events[0]->get_time(),
            (date::sys_days{date::year{2024} / 1 / 8}
             - date::sys_days{config()->get_simulation_timeframe().get_starting_date()})
                .count());
}

TEST_F(PopulationEventBuilderTest, ExecutesSimpleMutationAndMosquitoConfigurationEvents) {
  auto turn_on = std::make_unique<TurnOnMutationEvent>(0, 0.25);
  turn_on->set_executable(true);
  turn_on->execute();
  EXPECT_DOUBLE_EQ(config()->get_genotype_parameters().get_mutation_probability_per_locus(), 0.25);

  auto turn_off = std::make_unique<TurnOffMutationEvent>(0);
  turn_off->set_executable(true);
  turn_off->execute();
  EXPECT_DOUBLE_EQ(config()->get_genotype_parameters().get_mutation_probability_per_locus(), 0.0);

  auto recombination = std::make_unique<ChangeWithinHostInducedFreeRecombinationEvent>(false, 0);
  recombination->set_executable(true);
  recombination->execute();
  EXPECT_FALSE(config()->get_mosquito_parameters().get_within_host_induced_free_recombination());

  auto mask = std::make_unique<ChangeMutationMaskEvent>(std::vector<bool>{true, false}, 0);
  mask->set_executable(true);
  mask->execute();
  EXPECT_EQ(config()->get_genotype_parameters().get_mutation_mask(),
            std::vector<bool>({true, false}));

  auto ifr = std::make_unique<ChangeInterruptedFeedingRateEvent>(0, 0.42, 0);
  ifr->set_executable(true);
  ifr->execute();
  EXPECT_DOUBLE_EQ(config()->location_db()[0].mosquito_ifr, 0.42);

  auto probability = std::make_unique<ChangeMutationProbabilityPerLocusEvent>(0.33, 0);
  probability->set_executable(true);
  probability->execute();
  EXPECT_DOUBLE_EQ(config()->get_genotype_parameters().get_mutation_probability_per_locus(), 0.33);
}

TEST_F(PopulationEventBuilderTest, ExecutesAnnualAndCirculationUpdates) {
  config()->location_db()[0].beta = 1.0;
  auto annual_beta = std::make_unique<AnnualBetaUpdateEvent>(0.5, 0);
  annual_beta->set_executable(true);
  annual_beta->execute();
  EXPECT_DOUBLE_EQ(config()->location_db()[0].beta, 1.5);

  auto* treatment_coverage = Model::get_treatment_coverage();
  ASSERT_NE(treatment_coverage, nullptr);
  treatment_coverage->p_treatment_under_5[0] = 0.2;
  treatment_coverage->p_treatment_over_5[0] = 0.4;
  auto annual_coverage = std::make_unique<AnnualCoverageUpdateEvent>(0.5, 0);
  annual_coverage->set_executable(true);
  annual_coverage->execute();
  EXPECT_DOUBLE_EQ(treatment_coverage->p_treatment_under_5[0], 0.6);
  EXPECT_DOUBLE_EQ(treatment_coverage->p_treatment_over_5[0], 0.7);

  auto circulation_info = config()->get_movement_settings().get_circulation_info();
  circulation_info.set_circulation_percent(0.1);
  config()->get_movement_settings().set_circulation_info(circulation_info);
  auto circulation = std::make_unique<ChangeCirculationPercentEvent>(0.8, 0);
  circulation->set_executable(true);
  circulation->execute();
  EXPECT_NEAR(config()->get_movement_settings().get_circulation_info().get_circulation_percent(),
              0.8, 1e-6);
}
