#include <gtest/gtest.h>

#include <cstdio>

#include <spdlog/spdlog.h>

#include "Reporters/ValidationReporter.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Logger.h"
#include "fixtures/TestFileGenerators.h"

class ValidationReporterTest : public ::testing::Test {
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
    spdlog::drop_all();
    Logger::initialize(spdlog::level::off);
    test_fixtures::cleanup_test_files();
    for (const auto* name : {"validation_monthly_data_0.txt", "validation_summary_0.txt",
                             "validation_gene_freq_0.txt", "validation_gene_db_0.txt",
                             "validation_monthly_mutation_0.txt",
                             "validation_mosquito_res_count_0.txt"}) {
      std::remove(name);
    }
  }
};

TEST_F(ValidationReporterTest, RunsDirectReporterLifecycle) {
  ValidationReporter reporter;
  EXPECT_NO_THROW(reporter.initialize(0, "."));
  EXPECT_NO_THROW(reporter.before_run());
  EXPECT_NO_THROW(reporter.begin_time_step());
  EXPECT_NO_THROW(reporter.monthly_report());
  EXPECT_NO_THROW(reporter.after_run());
}

TEST_F(ValidationReporterTest, RunsRecombinationReporterLifecycle) {
  Model::get_config()->get_mosquito_parameters().set_record_recombination_events(true);
  ValidationReporter reporter;
  EXPECT_NO_THROW(reporter.initialize(0, "."));
  EXPECT_NO_THROW(reporter.before_run());
  EXPECT_NO_THROW(reporter.begin_time_step());
  EXPECT_NO_THROW(reporter.monthly_report());
  EXPECT_NO_THROW(reporter.after_run());
}

TEST_F(ValidationReporterTest, ReportsNonZeroPopulationAndTreatmentBranches) {
  auto* mdc = Model::get_mdc();
  mdc->popsize_by_location_hoststate()[0][Person::ASYMPTOMATIC] = 2;
  mdc->popsize_by_location_hoststate_age_class()[0][Person::ASYMPTOMATIC][0] = 1;
  mdc->number_of_clinical_by_location_age_group()[0][0] = 1;
  mdc->monthly_number_of_clinical_episode_by_location()[0] = 1;
  mdc->number_of_treatments_with_therapy_id()[0] = 2;
  mdc->number_of_treatments_success_with_therapy_id()[0] = 1;
  mdc->number_of_treatments_fail_with_therapy_id()[0] = 1;

  ValidationReporter reporter;
  ASSERT_NO_THROW(reporter.initialize(0, "."));
  ASSERT_NO_THROW(reporter.monthly_report());
}
