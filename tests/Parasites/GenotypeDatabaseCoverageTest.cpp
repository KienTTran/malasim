#include <gtest/gtest.h>

#include <cmath>

#include "Parasites/GenotypeDatabase.h"
#include "Parasites/Genotype.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class GenotypeDatabaseCoverageTest : public ::testing::Test {
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

TEST_F(GenotypeDatabaseCoverageTest, ReportsMinimumEc50ForConfiguredDrug) {
  auto *database = Model::get_genotype_db();
  const auto minimum_ec50 = database->get_min_ec50(0);

  EXPECT_TRUE(std::isfinite(minimum_ec50));
  EXPECT_GT(minimum_ec50, 0.0);
}

TEST_F(GenotypeDatabaseCoverageTest, KeepsInvalidGeneratedGenotypeInDatabase) {
  auto *database = Model::get_genotype_db();
  auto *genotype = database->get_genotype("invalid-genotype");

  ASSERT_NE(genotype, nullptr);
  EXPECT_EQ(database->get_id("invalid-genotype"), genotype->genotype_id());
}
