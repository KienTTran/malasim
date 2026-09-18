#include <gtest/gtest.h>

#include "Events/Population/AnnualBetaUpdateEvent.hxx"
#include "Events/Population/ChangeMutationMaskEvent.h"
#include "Events/Population/ChangeTreatmentCoverageEvent.h"
#include "Events/Population/ChangeTreatmentStrategyEvent.h"
#include "Events/Population/DistrictImportationDailyEvent.h"
#include "Events/Population/ImportationEvent.h"
#include "Events/Population/ImportationPeriodicallyEvent.h"
#include "Events/Population/ImportationPeriodicallyRandomEvent.h"
#include "Events/Population/IntroduceMutantEvent.hxx"
#include "Events/Population/ModifyNestedMFTEvent.h"
#include "Events/Population/PopulationEventBuilder.h"
#include "Events/Population/RotateStrategyEvent.h"
#include "Events/Population/SingleRoundMDAEvent.h"
#include "Events/Population/UpdateBetaRasterEvent.hxx"
#include "Parasites/Genotype.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventBuilderDispatchTest : public ::testing::Test {
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
};

TEST_F(PopulationEventBuilderDispatchTest, ExecutesZeroCaseImportationEventsAndReschedules) {
  auto importation = std::make_unique<ImportationEvent>(0, 0, 0, 0);
  importation->set_executable(true);
  EXPECT_NO_THROW(importation->execute());
  auto periodic = std::make_unique<ImportationPeriodicallyEvent>(0, 1, 0, 0, 0);
  periodic->set_executable(true);
  EXPECT_NO_THROW(periodic->execute());
  auto random_periodic = std::make_unique<ImportationPeriodicallyRandomEvent>(0, 0, 0, 1.0);
  random_periodic->set_executable(true);
  EXPECT_NO_THROW(random_periodic->execute());
}

TEST_F(PopulationEventBuilderDispatchTest, ExecutesStrategyAndDistrictEventsWithNoCases) {
  auto strategy_change = std::make_unique<ChangeTreatmentStrategyEvent>(0, 0);
  strategy_change->set_executable(true);
  EXPECT_NO_THROW(strategy_change->execute());
  ASSERT_NE(Model::get_treatment_strategy(), nullptr);
  EXPECT_EQ(Model::get_treatment_strategy()->id, 0);
  auto district = std::make_unique<DistrictImportationDailyEvent>(
      0, 0.0, 0, std::vector<std::tuple<int, int, char>>{});
  district->set_executable(true);
  EXPECT_NO_THROW(district->execute());
}

TEST_F(PopulationEventBuilderDispatchTest, DispatchesRemainingSupportedEventDefinitions) {
  const auto genotype_sequence = Model::get_genotype_db()->at(0)->get_aa_sequence();
  YAML::Node importation;
  importation["name"] = ImportationEvent::EVENT_NAME;
  importation["info"][0]["location"] = 0;
  importation["info"][0]["parasite_info"][0]["genotype_aa_sequence"] = genotype_sequence;
  importation["info"][0]["parasite_info"][0]["number_of_cases"] = 0;
  importation["info"][0]["parasite_info"][0]["date"] = "2024/01/01";
  EXPECT_EQ(PopulationEventBuilder::build(importation).size(), 1);

  YAML::Node periodic;
  periodic["name"] = ImportationPeriodicallyEvent::EVENT_NAME;
  periodic["info"][0]["location"] = 0;
  periodic["info"][0]["parasite_info"][0]["genotype_aa_sequence"] = genotype_sequence;
  periodic["info"][0]["parasite_info"][0]["duration"] = 1;
  periodic["info"][0]["parasite_info"][0]["number_of_cases"] = 0;
  periodic["info"][0]["parasite_info"][0]["start_date"] = "2024/01/01";
  EXPECT_EQ(PopulationEventBuilder::build(periodic).size(), 1);

  YAML::Node random_periodic;
  random_periodic["name"] = ImportationPeriodicallyRandomEvent::EVENT_NAME;
  random_periodic["info"][0]["date"] = "2024/02/01";
  random_periodic["info"][0]["genotype_id"] = 0;
  random_periodic["info"][0]["count"] = 1;
  random_periodic["info"][0]["log_parasite_density"] = 1.0;
  EXPECT_EQ(PopulationEventBuilder::build(random_periodic).size(), 1);

  YAML::Node annual_beta;
  annual_beta["name"] = AnnualBetaUpdateEvent::EVENT_NAME;
  annual_beta["info"][0]["date"] = "2024/01/01";
  annual_beta["info"][0]["rate"] = 0.1;
  EXPECT_EQ(PopulationEventBuilder::build(annual_beta).size(), 1);

  YAML::Node mask;
  mask["name"] = ChangeMutationMaskEvent::EVENT_NAME;
  mask["info"][0]["date"] = "2024/01/01";
  mask["info"][0]["mutation_mask"] = YAML::Load("[true, false]");
  EXPECT_EQ(PopulationEventBuilder::build(mask).size(), 1);

  YAML::Node unknown;
  unknown["name"] = "unsupported_event";
  EXPECT_TRUE(PopulationEventBuilder::build(unknown).empty());
}

TEST_F(PopulationEventBuilderDispatchTest, DispatchesConfigurationAndRasterEventDefinitions) {
  YAML::Node coverage;
  coverage["name"] = ChangeTreatmentCoverageEvent::EVENT_NAME;
  coverage["info"][0] = YAML::Load(R"(
type: SteadyTCM
date: 2000/1/1
p_treatment_under_5_by_location: [0.1]
p_treatment_over_5_by_location: [0.2]
)");
  EXPECT_EQ(PopulationEventBuilder::build(coverage).size(), 1);

  YAML::Node strategy;
  strategy["name"] = ChangeTreatmentStrategyEvent::EVENT_NAME;
  strategy["info"][0]["date"] = "2000/1/1";
  strategy["info"][0]["strategy_id"] = 0;
  EXPECT_EQ(PopulationEventBuilder::build(strategy).size(), 1);

  YAML::Node single_mda;
  single_mda["name"] = SingleRoundMDAEvent::EVENT_NAME;
  single_mda["info"][0]["date"] = "2000/1/1";
  single_mda["info"][0]["fraction_population_targeted"] = YAML::Load("[0.0]");
  single_mda["info"][0]["days_to_complete_all_treatments"] = 1;
  EXPECT_EQ(PopulationEventBuilder::build(single_mda).size(), 1);

  YAML::Node nested;
  nested["name"] = ModifyNestedMFTEvent::EVENT_NAME;
  nested["info"][0]["date"] = "2000/1/1";
  nested["info"][0]["strategy_id"] = 0;
  EXPECT_EQ(PopulationEventBuilder::build(nested).size(), 1);

  YAML::Node raster;
  raster["name"] = UpdateBetaRasterEvent::EVENT_NAME;
  raster["info"][0]["date"] = "2000/1/1";
  raster["info"][0]["beta_raster"] = "builder_beta_dispatch.asc";
  test_fixtures::create_test_raster_file("builder_beta_dispatch.asc", 1, 1, 0.5);
  EXPECT_EQ(PopulationEventBuilder::build(raster).size(), 1);
  std::remove("builder_beta_dispatch.asc");

  YAML::Node rotate;
  rotate["name"] = RotateStrategyEvent::EVENT_NAME;
  rotate["info"][0]["date"] = "2000/1/1";
  rotate["info"][0]["years"] = 1;
  rotate["info"][0]["first_strategy_id"] = 0;
  rotate["info"][0]["second_strategy_id"] = 0;
  EXPECT_EQ(PopulationEventBuilder::build(rotate).size(), 1);

  YAML::Node district;
  district["name"] = DistrictImportationDailyEvent::EVENT_NAME;
  district["info"][0]["district"] = 1;
  district["info"][0]["daily_rate"] = 0.1;
  district["info"][0]["start_date"] = "2000/1/1";
  district["info"][0]["alleles"] = YAML::Load("[]");
  EXPECT_EQ(PopulationEventBuilder::build(district).size(), 1);

  YAML::Node mutant;
  mutant["name"] = IntroduceMutantEvent::EVENT_NAME;
  mutant["admin_level"] = "district";
  mutant["info"] = YAML::Load(R"(
- day: 2000/1/1
  unit_id: 0
  fraction: 0.1
  alleles:
    - chromosome: 1
      locus: 2
      allele: A
)");
  EXPECT_EQ(PopulationEventBuilder::build(mutant).size(), 1);
}

TEST_F(PopulationEventBuilderDispatchTest, LogsAndSkipsMultiCharacterMutantAlleles) {
  const auto node = YAML::Load(R"(
- location: 0
  date: 2024/01/01
  fraction: 0.2
  alleles:
    - chromosome: 5
      locus: 86
      allele: AA
)");
  EXPECT_EQ(PopulationEventBuilder::build_introduce_amodiaquine_mutant_parasite_events(
                node, Model::get_config())
                .size(),
            1U);
  EXPECT_EQ(PopulationEventBuilder::build_introduce_lumefantrine_mutant_parasite_events(
                node, Model::get_config())
                .size(),
            1U);
  EXPECT_EQ(PopulationEventBuilder::build_introduce_580Y_mutant_events(node, Model::get_config())
                .size(),
            1U);
  EXPECT_EQ(PopulationEventBuilder::build_introduce_triple_mutant_to_dpm_parasite_events(
                node, Model::get_config())
                .size(),
            1U);
}

TEST_F(PopulationEventBuilderDispatchTest, FiltersOutOfRangeLocationDefinitions) {
  const auto out_of_range = Model::get_config()->number_of_locations() + 1;
  YAML::Node location_only;
  location_only[0]["location"] = out_of_range;
  EXPECT_TRUE(PopulationEventBuilder::build_introduce_parasite_events(
                  location_only, Model::get_config())
                  .empty());
  EXPECT_TRUE(PopulationEventBuilder::build_introduce_parasites_periodically_events(
                  location_only, Model::get_config())
                  .empty());
  EXPECT_TRUE(PopulationEventBuilder::build_introduce_plas2_parasite_events(
                  location_only, Model::get_config())
                  .empty());
  EXPECT_TRUE(PopulationEventBuilder::build_change_interrupted_feeding_rate_event(
                  location_only, Model::get_config())
                  .empty());
  EXPECT_TRUE(PopulationEventBuilder::build_introduce_amodiaquine_mutant_parasite_events(
                  location_only, Model::get_config())
                  .empty());
  EXPECT_TRUE(PopulationEventBuilder::build_introduce_lumefantrine_mutant_parasite_events(
                  location_only, Model::get_config())
                  .empty());
  EXPECT_TRUE(PopulationEventBuilder::build_introduce_580Y_mutant_events(
                  location_only, Model::get_config())
                  .empty());
  EXPECT_TRUE(PopulationEventBuilder::build_introduce_triple_mutant_to_dpm_parasite_events(
                  location_only, Model::get_config())
                  .empty());
}

TEST_F(PopulationEventBuilderDispatchTest, CoversSentinelPeriodicAndValidationBranches) {
  const auto periodic_v2 = YAML::Load(R"(
    - location: -1
      parasite_info:
        - duration: 2
          number_of_cases: 0
          start_date: 2000/1/1
          end_date: 2000/2/1
  )");
  EXPECT_EQ(PopulationEventBuilder::build_introduce_parasites_periodically_events_v2(
                periodic_v2, Model::get_config())
                .size(),
            Model::get_config()->number_of_locations());

  const auto invalid_circulation = YAML::Load(R"(
    - date: 2000/1/1
      circulation_percent: -0.1
  )");
  EXPECT_THROW(PopulationEventBuilder::build_change_circulation_percent_event(
                   invalid_circulation, Model::get_config()),
               std::invalid_argument);

  const auto invalid_rotation = YAML::Load(R"(
    - date: 2000/1/1
      years: 0
      first_strategy_id: 0
      second_strategy_id: 0
  )");
  EXPECT_THROW(PopulationEventBuilder::build_rotate_treatment_strategy_event(
                   invalid_rotation, Model::get_config()),
               std::invalid_argument);

  YAML::Node invalid_annual;
  invalid_annual.push_back(YAML::Node());
  invalid_annual.push_back(YAML::Node());
  EXPECT_THROW(PopulationEventBuilder::build_annual_beta_update_event(
                   invalid_annual, Model::get_config()),
               std::invalid_argument);
}
