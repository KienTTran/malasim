#include <gtest/gtest.h>

#include "MDC/ModelDataCollector.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person/Person.h"
#include "Treatment/Therapies/SCTherapy.h"
#include "Parasites/Genotype.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ModelDataCollectorBehaviorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
    collector_ = Model::get_mdc();
    collector_->initialize();
    collector_->set_recording(true);
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  ModelDataCollector* collector_{nullptr};
};

TEST_F(ModelDataCollectorBehaviorTest, RecordsTreatmentFailureDeathInfectionAndRecrudescence) {
  constexpr core::LocationId location = 0;
  collector_->record_1_recrudescence_treatment(location, 85, 1, 0);
  collector_->record_1_non_treated_case(location, 85, 1);
  collector_->record_1_malaria_death(location, 85, true);
  collector_->record_1_malaria_death(location, 25, false);
  collector_->record_1_infection(location);
  collector_->record_1_treatment_failure_by_therapy(location, 1, 0);
  collector_->record_1_treatment_success_by_therapy(location, 1, 0);
  collector_->update_utl_vector();

  EXPECT_EQ(collector_->monthly_number_of_recrudescence_treatment_by_location()[location], 1);
  EXPECT_EQ(collector_->monthly_nontreatment_by_location()[location], 1);
  EXPECT_EQ(collector_->monthly_number_of_new_infections_by_location()[location], 1);
  EXPECT_EQ(collector_->monthly_treatment_failure_by_location()[location], 1);
  EXPECT_EQ(collector_->monthly_treatment_success_by_location()[location], 1);
  EXPECT_NO_THROW(collector_->monthly_update());
}

TEST_F(ModelDataCollectorBehaviorTest, ComputesYearlyAndPartialYearEir) {
  constexpr core::LocationId location = 0;
  auto* scheduler = Model::get_scheduler();
  scheduler->set_current_time(0);
  collector_->yearly_update();

  collector_->collect_number_of_bites(location, 365);
  collector_->update_person_days_by_years(location, 365);
  scheduler->set_current_time(365);
  EXPECT_NO_THROW(collector_->yearly_update());
  EXPECT_NO_THROW(collector_->calculate_eir());
  EXPECT_NO_THROW(collector_->update_after_run());
}

TEST_F(ModelDataCollectorBehaviorTest, HandlesBloodSlideRangesAndMutationByDrug) {
  constexpr core::LocationId location = 0;
  EXPECT_DOUBLE_EQ(collector_->get_blood_slide_prevalence(location, 0, 5), 0.0);
  EXPECT_DOUBLE_EQ(collector_->get_blood_slide_prevalence(location, 0, 20), 0.0);
  EXPECT_DOUBLE_EQ(collector_->get_blood_slide_prevalence(location, 10, 20), 0.0);

  auto from = Model::get_genotype_db()->at(0);
  auto to = Model::get_genotype_db()->at(0);
  EXPECT_NO_THROW(collector_->record_1_mutation_by_drug(location, from, to, 0));
}

TEST_F(ModelDataCollectorBehaviorTest, RecordsTreatmentSeekingByConfiguredAgeIndex) {
  constexpr core::LocationId location = 0;
  collector_->record_1_treatment(location, 1, 0, 0);
  collector_->record_1_treatment(location, 85, 0, 0);

  const auto& counts = collector_->monthly_number_of_people_seeking_treatment_by_location_age_index();
  ASSERT_GE(counts.size(), 1U);
  ASSERT_EQ(counts[location].size(), 7U);
  EXPECT_EQ(counts[location][0], 1);
  EXPECT_EQ(counts[location].back(), 1);
}

TEST_F(ModelDataCollectorBehaviorTest, YearlyUpdateHandlesZeroPersonDays) {
  constexpr core::LocationId location = 0;
  auto* scheduler = Model::get_scheduler();
  scheduler->set_current_time(0);
  collector_->yearly_update();
  collector_->set_person_days_by_location_year(LongVector(8, 0));
  scheduler->set_current_time(365);
  EXPECT_NO_THROW(collector_->yearly_update());
  ASSERT_GE(collector_->eir_by_location_year().size(), 1U);
  ASSERT_EQ(collector_->eir_by_location_year()[location].size(), 1U);
  EXPECT_DOUBLE_EQ(collector_->eir_by_location_year()[location][0], 0.0);
}

TEST_F(ModelDataCollectorBehaviorTest, ClampsAndDefaultsTreatmentSeekingAgeIndex) {
  constexpr core::LocationId location = 0;
  collector_->set_monthly_number_of_people_seeking_treatment_by_location_age_index({{0}});
  collector_->record_1_treatment(location, 100, 0, 0);
  EXPECT_EQ(collector_->monthly_number_of_people_seeking_treatment_by_location_age_index()
                [location][0],
            1);

  auto age_config = Model::get_config()
                        ->get_epidemiological_parameters()
                        .get_age_based_probability_of_seeking_treatment();
  age_config.set_ages({});
  Model::get_config()->get_epidemiological_parameters().set_age_based_probability_of_seeking_treatment(
      age_config);
  collector_->record_1_treatment(location, 20, 0, 0);
  EXPECT_EQ(collector_->monthly_number_of_people_seeking_treatment_by_location_age_index()
                [location][0],
            2);
}

TEST_F(ModelDataCollectorBehaviorTest, RecordsArtemisininCombinationResistanceMetrics) {
  Person person;
  auto amu_genotype = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
  auto afu_genotype = std::make_unique<Genotype>("||||YF2||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
  amu_genotype->EC50_power_n.assign(Model::get_drug_db()->size(), 0.0);
  afu_genotype->EC50_power_n.assign(Model::get_drug_db()->size(), 1.0e6);
  if (Model::get_drug_db()->size() < 2) GTEST_SKIP() << "fixture requires two drugs";
  amu_genotype->EC50_power_n[1] = 1.0e6;

  auto amu = std::make_unique<ClonalParasitePopulation>(amu_genotype.get());
  auto afu = std::make_unique<ClonalParasitePopulation>(afu_genotype.get());
  auto* amu_ptr = amu.get();
  person.get_all_clonal_parasite_populations()->add(std::move(amu));
  person.get_all_clonal_parasite_populations()->add(std::move(afu));

  SCTherapy therapy;
  therapy.artemisinin_id = 0;
  therapy.drug_ids = {0, 1};
  therapy.dosing_day = {3};
  therapy.calculate_max_dosing_day();
  Model::get_scheduler()->set_current_time(
      Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period());
  collector_->record_amu_afu(&person, &therapy, amu_ptr);

  EXPECT_GT(collector_->amu_per_person(), 0.0);
  EXPECT_GT(collector_->afu(), 0.0);
  EXPECT_GT(collector_->amu_for_clinical_caused_parasite(), 0.0);
}
