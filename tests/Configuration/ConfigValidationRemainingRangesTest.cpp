#include <gtest/gtest.h>

#define private public
#include "Configuration/Config.h"
#undef private

#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ConfigValidationRemainingRangesTest : public ::testing::Test {
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

TEST_F(ConfigValidationRemainingRangesTest, CoversTransmissionAndTreatmentRangeChecks) {
  auto &epi = config()->epidemiological_parameters_;
  epi.days_mature_gametocyte_under_five_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);
  epi.days_mature_gametocyte_under_five_ = 1;
  epi.p_compliance_ = 2;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);
  epi.p_compliance_ = 1;
  epi.min_dosing_days_ = 10;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);
  epi.min_dosing_days_ = 1;
  epi.gametocyte_level_under_artemisinin_action_ = 2;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);
  epi.gametocyte_level_under_artemisinin_action_ = 0.5;
  epi.p_relapse_ = 2;
  EXPECT_THROW(config()->validate_epidemiological_treatment_parameters(10), std::invalid_argument);

  epi.p_relapse_ = 0.1;
  epi.relapse_rate_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
  epi.relapse_rate_ = 0.1;
  epi.update_frequency_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
  epi.update_frequency_ = 1;
  epi.tf_window_size_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
  epi.tf_window_size_ = 60;
  epi.fraction_mosquitoes_interrupted_feeding_ = 2;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
  epi.fraction_mosquitoes_interrupted_feeding_ = 0.1;
  epi.inflation_factor_ = -1;
  EXPECT_THROW(config()->validate_epidemiological_transmission_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationRemainingRangesTest, CoversParasiteAndDrugValidationRanges) {
  auto &parasite = config()->parasite_parameters_;
  parasite.parasite_density_levels_.log_parasite_density_cured_ = 6;
  EXPECT_THROW(config()->validate_parasite_parameters(), std::invalid_argument);
  parasite.parasite_density_levels_.log_parasite_density_cured_ = 1;
  parasite.recombination_parameters_.within_chromosome_recombination_rate_ = 2;
  EXPECT_THROW(config()->validate_parasite_parameters(), std::invalid_argument);

  auto drugs = config()->drug_parameters_.get_drug_db_raw();
  ASSERT_FALSE(drugs.empty());
  auto drug = drugs.begin()->second;
  drug.set_half_life(-1);
  drugs.begin()->second = drug;
  config()->drug_parameters_.set_drug_db_raw(drugs);
  EXPECT_THROW(config()->validate_drug_parameters(), std::invalid_argument);

  drug.set_half_life(1);
  drug.set_maximum_parasite_killing_rate(2);
  drugs.begin()->second = drug;
  config()->drug_parameters_.set_drug_db_raw(drugs);
  EXPECT_THROW(config()->validate_drug_parameters(), std::invalid_argument);
}

TEST_F(ConfigValidationRemainingRangesTest, CoversInitialGenotypeSequenceValidation) {
  auto &genotype = config()->genotype_parameters_;
  ASSERT_FALSE(genotype.mutation_mask_.empty());
  GenotypeParameters::InitialParasiteInfoRaw initial;
  initial.location_id_ = 0;
  GenotypeParameters::ParasiteInfo parasite;
  parasite.aa_sequence_ = "invalid";
  initial.parasite_info_.push_back(parasite);
  genotype.initial_parasite_info_raw_ = {initial};
  EXPECT_THROW(config()->validate_genotype_parameters(), std::invalid_argument);

  parasite.aa_sequence_ = "|||||||||||||";
  initial.parasite_info_ = {parasite};
  genotype.initial_parasite_info_raw_ = {initial};
  EXPECT_THROW(config()->validate_genotype_parameters(), std::invalid_argument);
}
