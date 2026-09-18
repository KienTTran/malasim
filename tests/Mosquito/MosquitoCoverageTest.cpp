#include <gtest/gtest.h>

#include <memory>

#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Mosquito/Mosquito.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class MosquitoCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
    person_ = std::make_unique<Person>();
    person_->initialize();
    genotype_ = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
  }

  void TearDown() override {
    person_.reset();
    genotype_.reset();
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<Person> person_;
  std::unique_ptr<Genotype> genotype_;
};

TEST_F(MosquitoCoverageTest, SelectsOnlyGametocytaemicParasiteProfiles) {
  auto infectious = std::make_unique<ClonalParasitePopulation>(genotype_.get());
  infectious->set_gametocyte_level(1.0);
  infectious->set_last_update_log10_parasite_density(3.0);
  auto noninfectious = std::make_unique<ClonalParasitePopulation>(genotype_.get());
  noninfectious->set_gametocyte_level(0.0);
  noninfectious->set_last_update_log10_parasite_density(3.0);
  person_->get_all_clonal_parasite_populations()->add(std::move(infectious));
  person_->get_all_clonal_parasite_populations()->add(std::move(noninfectious));

  Mosquito mosquito;
  std::vector<Genotype*> genotypes;
  std::vector<double> infectivity;
  mosquito.get_genotypes_profile_from_person(person_.get(), genotypes, infectivity);

  ASSERT_EQ(genotypes.size(), 1U);
  ASSERT_EQ(infectivity.size(), 1U);
  EXPECT_EQ(genotypes.front(), genotype_.get());
  EXPECT_GT(infectivity.front(), 0.0);
}
