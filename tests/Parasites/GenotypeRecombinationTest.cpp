#include <gtest/gtest.h>

#include "Parasites/Genotype.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

namespace {
class FixedUniformRandom final : public utils::Random {
 public:
  explicit FixedUniformRandom(double value) : utils::Random(nullptr, 1234), value_(value) {}
  double random_uniform() override { return value_; }

 private:
  double value_;
};
}  // namespace

class GenotypeRecombinationTest : public ::testing::Test {
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

TEST_F(GenotypeRecombinationTest, FreeRecombinationProducesDatabaseGenotype) {
  const auto aa_sequence = Model::get_genotype_db()->at(0)->get_aa_sequence();
  Genotype female(aa_sequence);
  Genotype male(aa_sequence);
  FixedUniformRandom random(0.25);

  auto* result = Genotype::free_recombine(Model::get_config(), &random, &female, &male);

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_aa_sequence(), aa_sequence);
}

TEST_F(GenotypeRecombinationTest, HandlesMultipleGenesWithAndWithoutCrossover) {
  // This fixture genotype contains the multi-gene chromosome used by the
  // production template (`TTHFIMG,x`), so both crossover branches are real.
  const std::string aa_sequence = "||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1";
  Genotype female(aa_sequence);
  Genotype male(aa_sequence);

  auto recombination_parameters =
      Model::get_config()->get_parasite_parameters().get_recombination_parameters();
  recombination_parameters.set_within_chromosome_recombination_rate(1.0);
  Model::get_config()->get_parasite_parameters().set_recombination_parameters(
      recombination_parameters);
  FixedUniformRandom crossover_random(0.0);
  EXPECT_NE(Genotype::free_recombine(Model::get_config(), &crossover_random, &female, &male),
            nullptr);

  recombination_parameters.set_within_chromosome_recombination_rate(0.0);
  Model::get_config()->get_parasite_parameters().set_recombination_parameters(
      recombination_parameters);
  FixedUniformRandom no_crossover_random(0.99);
  EXPECT_NE(Genotype::free_recombine(Model::get_config(), &no_crossover_random, &female, &male),
            nullptr);

  // Keep the shared model configuration isolated from tests that follow.
  recombination_parameters.set_within_chromosome_recombination_rate(0.1);
  Model::get_config()->get_parasite_parameters().set_recombination_parameters(
      recombination_parameters);
}

TEST_F(GenotypeRecombinationTest, MemberRecombinationHandlesCrossoverBranches) {
  const std::string aa_sequence = "||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1";
  Genotype female(aa_sequence);
  Genotype male(aa_sequence);
  auto parameters = Model::get_config()->get_parasite_parameters().get_recombination_parameters();

  parameters.set_within_chromosome_recombination_rate(1.0);
  Model::get_config()->get_parasite_parameters().set_recombination_parameters(parameters);
  FixedUniformRandom crossover_random(0.0);
  EXPECT_NE(female.free_recombine_with(Model::get_config(), &crossover_random, &male), nullptr);

  parameters.set_within_chromosome_recombination_rate(0.0);
  Model::get_config()->get_parasite_parameters().set_recombination_parameters(parameters);
  FixedUniformRandom no_crossover_random(0.99);
  EXPECT_NE(female.free_recombine_with(Model::get_config(), &no_crossover_random, &male), nullptr);

  parameters.set_within_chromosome_recombination_rate(0.1);
  Model::get_config()->get_parasite_parameters().set_recombination_parameters(parameters);
}
