#include <gtest/gtest.h>

#include "MDC/ModelDataCollector.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class ModelDataCollectorScalarAccessorsTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
    collector_ = Model::get_mdc();
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  ModelDataCollector* collector_ = nullptr;
};

TEST_F(ModelDataCollectorScalarAccessorsTest, RoundTripsAllScalarStatistics) {
  collector_->set_amu_per_parasite_pop(1.0);
  collector_->set_amu_per_person(2.0);
  collector_->set_amu_for_clinical_caused_parasite(3.0);
  collector_->set_afu(4.0);
  collector_->set_discounted_amu_per_parasite_pop(5.0);
  collector_->set_discounted_amu_per_person(6.0);
  collector_->set_discounted_amu_for_clinical_caused_parasite(7.0);
  collector_->set_discounted_afu(8.0);
  EXPECT_DOUBLE_EQ(collector_->amu_per_parasite_pop(), 1.0);
  EXPECT_DOUBLE_EQ(collector_->amu_per_person(), 2.0);
  EXPECT_DOUBLE_EQ(collector_->amu_for_clinical_caused_parasite(), 3.0);
  EXPECT_DOUBLE_EQ(collector_->afu(), 4.0);
  EXPECT_DOUBLE_EQ(collector_->discounted_amu_per_parasite_pop(), 5.0);
  EXPECT_DOUBLE_EQ(collector_->discounted_amu_per_person(), 6.0);
  EXPECT_DOUBLE_EQ(collector_->discounted_amu_for_clinical_caused_parasite(), 7.0);
  EXPECT_DOUBLE_EQ(collector_->discounted_afu(), 8.0);

  collector_->set_tf_at_15(9.0);
  collector_->set_single_resistance_frequency_at_15(10.0);
  collector_->set_double_resistance_frequency_at_15(11.0);
  collector_->set_triple_resistance_frequency_at_15(12.0);
  collector_->set_quadruple_resistance_frequency_at_15(13.0);
  collector_->set_quintuple_resistance_frequency_at_15(14.0);
  collector_->set_art_resistance_frequency_at_15(15.0);
  collector_->set_total_resistance_frequency_at_15(16.0);
  EXPECT_DOUBLE_EQ(collector_->tf_at_15(), 9.0);
  EXPECT_DOUBLE_EQ(collector_->single_resistance_frequency_at_15(), 10.0);
  EXPECT_DOUBLE_EQ(collector_->double_resistance_frequency_at_15(), 11.0);
  EXPECT_DOUBLE_EQ(collector_->triple_resistance_frequency_at_15(), 12.0);
  EXPECT_DOUBLE_EQ(collector_->quadruple_resistance_frequency_at_15(), 13.0);
  EXPECT_DOUBLE_EQ(collector_->quintuple_resistance_frequency_at_15(), 14.0);
  EXPECT_DOUBLE_EQ(collector_->art_resistance_frequency_at_15(), 15.0);
  EXPECT_DOUBLE_EQ(collector_->total_resistance_frequency_at_15(), 16.0);

  collector_->set_current_number_of_mutation_events(17);
  EXPECT_EQ(collector_->current_number_of_mutation_events(), 17);
  collector_->set_recording(true);
  EXPECT_TRUE(collector_->recording_data());
}
