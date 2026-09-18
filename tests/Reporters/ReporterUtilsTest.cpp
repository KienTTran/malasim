#include <gtest/gtest.h>

#include <cstdio>
#include <sstream>

#include "Reporters/Utility/ReporterUtils.h"
#include "Simulation/Model.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

TEST(ReporterUtilsTest, EmptyPopulationProducesZeroFrequencyRecords) {
  PersonIndexByLocationStateAgeClass person_index(1, Person::NUMBER_OF_STATE, 1);
  std::stringstream genotype_frequency_1;
  std::stringstream genotype_frequency_2;
  std::stringstream genotype_frequency_3;
  std::stringstream combined_frequency;

  ReporterUtils::output_genotype_frequency1(genotype_frequency_1, 2, &person_index);
  ReporterUtils::output_genotype_frequency2(genotype_frequency_2, 2, &person_index);
  ReporterUtils::output_genotype_frequency3(genotype_frequency_3, 2, &person_index);
  ReporterUtils::output_3_genotype_frequency(combined_frequency, 2, &person_index);

  EXPECT_NE(genotype_frequency_1.str().find("0"), std::string::npos);
  EXPECT_NE(genotype_frequency_2.str().find("0"), std::string::npos);
  EXPECT_NE(genotype_frequency_3.str().find("0"), std::string::npos);
  EXPECT_NE(combined_frequency.str().find("0"), std::string::npos);
}

class ReporterUtilsModelTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    cli_input.job_number = 0;
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
    ReporterUtils::initialize_moi_file_logger();
  }

  void TearDown() override {
    Model::get_instance()->release();
    std::remove("moi_0.txt");
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(ReporterUtilsModelTest, EmptyPopulationSupportsMosquitoAndMoiReports) {
  PersonIndexByLocationStateAgeClass person_index(1, Person::NUMBER_OF_STATE, 1);
  std::stringstream genotype_frequency_4;
  std::stringstream prmc_frequency;
  std::stringstream moi;

  EXPECT_NO_THROW(ReporterUtils::output_genotype_frequency4(
      genotype_frequency_4, prmc_frequency, 1, &person_index));
  EXPECT_NO_THROW(ReporterUtils::output_moi(moi, &person_index));
  EXPECT_TRUE(moi.str().empty());
}

TEST_F(ReporterUtilsModelTest, ReportsGenotypeFrequenciesForAnInfectedPerson) {
  auto *pi = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  Person *infected = nullptr;
  for (auto &location : pi->vPerson()) {
    for (auto state = 0; state < Person::NUMBER_OF_STATE - 1 && infected == nullptr; ++state) {
      for (auto &age_class : location[state]) {
        if (!age_class.empty()) {
          infected = age_class.front();
          break;
        }
      }
    }
    if (infected != nullptr) break;
  }
  ASSERT_NE(infected, nullptr);
  infected->add_new_parasite_to_blood(Model::get_genotype_db()->at(0));

  std::stringstream frequency1;
  std::stringstream frequency2;
  std::stringstream frequency3;
  std::stringstream frequency4;
  std::stringstream prmc;
  std::stringstream combined;
  std::stringstream moi;

  ReporterUtils::output_genotype_frequency1(frequency1, 2, pi);
  ReporterUtils::output_genotype_frequency2(frequency2, 2, pi);
  ReporterUtils::output_genotype_frequency3(frequency3, 2, pi);
  ReporterUtils::output_genotype_frequency4(frequency4, prmc, 2, pi);
  ReporterUtils::output_3_genotype_frequency(combined, 2, pi);
  ReporterUtils::output_moi(moi, pi);

  EXPECT_FALSE(frequency1.str().empty());
  EXPECT_FALSE(frequency2.str().empty());
  EXPECT_FALSE(frequency3.str().empty());
  EXPECT_FALSE(frequency4.str().empty());
  EXPECT_FALSE(combined.str().empty());
  EXPECT_TRUE(moi.str().empty());
}
