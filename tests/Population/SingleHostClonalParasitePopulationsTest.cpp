#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <set>

#include "Core/types.h"
#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/DrugsInBlood.h"
#include "Population/Person/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Constants.h"
#include "fixtures/TestFileGenerators.h"

// Mock Person class
class MockPerson : public Person {
public:
  MockPerson() : Person() {}
  MOCK_METHOD(core::SimDay, get_latest_update_time, (), (const, override));
};

class SingleHostClonalParasitePopulationsTest : public ::testing::Test {
protected:
  void SetUp() override {
    person = std::make_unique<MockPerson>();
    populations = std::make_unique<SingleHostClonalParasitePopulations>(person.get());
    genotype = std::make_unique<Genotype>("abcdef");
    destroyed_parasites_.clear();
  }

  void TearDown() override {
    populations.reset();
    person.reset();
    genotype.reset();
  }

  std::unique_ptr<MockPerson> person;
  std::unique_ptr<SingleHostClonalParasitePopulations> populations;
  std::unique_ptr<Genotype> genotype;
  static std::set<int> destroyed_parasites_;

  // Helper class to track parasite destruction
  class TrackedParasite : public ClonalParasitePopulation {
  public:
    TrackedParasite(int tracking_id) : tracking_id_(tracking_id) {}

    ~TrackedParasite() override { destroyed_parasites_.insert(tracking_id_); }

    int tracking_id() const { return tracking_id_; }

  private:
    int tracking_id_;
  };

  // Helper method to create a trackable parasite
  std::unique_ptr<TrackedParasite> create_tracked_parasite(int tracking_id) {
    auto parasite = std::make_unique<TrackedParasite>(tracking_id);
    // Set any necessary default values
    parasite->set_last_update_log10_parasite_density(2.0);  // Above cured threshold
    return parasite;
  }

  static bool is_destroyed(int tracking_id) {
    return destroyed_parasites_.find(tracking_id) != destroyed_parasites_.end();
  }
};

std::set<int> SingleHostClonalParasitePopulationsTest::destroyed_parasites_;

TEST_F(SingleHostClonalParasitePopulationsTest, Initialization) {
  EXPECT_EQ(populations->size(), 0);
  EXPECT_TRUE(populations->empty());
  EXPECT_EQ(populations->person(), person.get());
  EXPECT_EQ(populations->log10_total_infectious_density(),
            SingleHostClonalParasitePopulations::DEFAULT_LOG_DENSITY);
}

TEST_F(SingleHostClonalParasitePopulationsTest, AddAndRemoveParasite) {
  auto parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite_ptr = parasite.get();
  // Add parasite
  populations->add(std::move(parasite));

  EXPECT_EQ(populations->size(), 1);
  EXPECT_FALSE(populations->empty());
  EXPECT_EQ(populations->at(0), parasite_ptr);
  EXPECT_EQ(parasite_ptr->get_index(), 0);

  // Remove parasite
  populations->remove(parasite_ptr->get_index());
  EXPECT_EQ(populations->size(), 0);
  EXPECT_TRUE(populations->empty());
}

TEST_F(SingleHostClonalParasitePopulationsTest, RemoveByIndex) {
  auto parasite1 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite2 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite2_ptr = parasite2.get();

  // Add two parasites
  populations->add(std::move(parasite1));
  populations->add(std::move(parasite2));
  EXPECT_EQ(populations->size(), 2);

  // Remove first parasite
  populations->remove(0);
  EXPECT_EQ(populations->size(), 1);
  EXPECT_EQ(populations->at(0), parasite2_ptr);
  EXPECT_EQ(parasite2_ptr->get_index(), 0);
}

TEST_F(SingleHostClonalParasitePopulationsTest, RemoveByIndexEdgeCases) {
  // Test removing from empty population
  EXPECT_THROW(populations->remove(0), std::out_of_range);

  // Test removing with negative index
  EXPECT_THROW(populations->remove(-1), std::out_of_range);

  // Test removing with index beyond size
  auto parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());
  populations->add(std::move(parasite));
  EXPECT_THROW(populations->remove(1), std::out_of_range);
}

TEST_F(SingleHostClonalParasitePopulationsTest, ContainParasite) {
  auto parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite_ptr = parasite.get();
  auto other_parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto other_parasite_ptr = other_parasite.get();

  populations->add(std::move(parasite));

  EXPECT_TRUE(populations->contain(parasite_ptr));
  EXPECT_FALSE(populations->contain(other_parasite_ptr));
  EXPECT_FALSE(populations->contain(nullptr));
}

TEST_F(SingleHostClonalParasitePopulationsTest, ClearCuredParasites) {
  auto parasite1 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite1_ptr = parasite1.get();
  auto parasite2 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite2_ptr = parasite2.get();
  auto parasite3 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite3_ptr = parasite3.get();

  // Set up parasites with different densities
  parasite1->set_last_update_log10_parasite_density(5.0);
  parasite2->set_last_update_log10_parasite_density(
      Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
  parasite3->set_last_update_log10_parasite_density(3.0);

  populations->add(std::move(parasite1));
  populations->add(std::move(parasite2));
  populations->add(std::move(parasite3));

  populations->clear_cured_parasites(Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);

  EXPECT_EQ(populations->size(), 2);
  EXPECT_EQ(populations->at(0), parasite1_ptr);
  EXPECT_EQ(populations->at(1), parasite3_ptr);
}

TEST_F(SingleHostClonalParasitePopulationsTest, ClearCuredParasitesEdgeCases) {
  // Test with empty population
  populations->clear_cured_parasites(Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
  EXPECT_EQ(populations->size(), 0);

  // Test with all cured parasites
  auto parasite1 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite2 = std::make_unique<ClonalParasitePopulation>(genotype.get());

  parasite1->set_last_update_log10_parasite_density(
      Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
  parasite2->set_last_update_log10_parasite_density(
      Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);

  populations->add(std::move(parasite1));
  populations->add(std::move(parasite2));

  populations->clear_cured_parasites(Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
  EXPECT_EQ(populations->size(), 0);
}

TEST_F(SingleHostClonalParasitePopulationsTest, HasDetectableParasite) {
  auto parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite_ptr = parasite.get();

  // Test with undetectable parasite
  parasite_ptr->set_last_update_log10_parasite_density(
      Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
  populations->add(std::move(parasite));
  EXPECT_FALSE(populations->has_detectable_parasite(1.0));

  // Test with detectable parasite
  parasite_ptr->set_last_update_log10_parasite_density(1.0);
  EXPECT_TRUE(populations->has_detectable_parasite(1.0));

  // Test with empty population
  populations->clear();
  EXPECT_FALSE(populations->has_detectable_parasite(1.0));
}

TEST_F(SingleHostClonalParasitePopulationsTest, IsGametocytaemic) {
  auto parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite_ptr = parasite.get();
  // Test with no gametocytes
  parasite_ptr->set_gametocyte_level(0.0);
  populations->add(std::move(parasite));
  EXPECT_FALSE(populations->is_gametocytaemic());

  // Test with gametocytes
  parasite_ptr->set_gametocyte_level(0.5);
  EXPECT_TRUE(populations->is_gametocytaemic());

  // Test with empty population
  populations->clear();
  EXPECT_FALSE(populations->is_gametocytaemic());
}

TEST_F(SingleHostClonalParasitePopulationsTest, UpdateWithDrugEffectsEmptyPopulation) {
  auto drugs_in_blood = std::make_unique<DrugsInBlood>();

  EXPECT_NO_THROW(populations->update_with_drug_effects(drugs_in_blood.get()));
}

TEST_F(SingleHostClonalParasitePopulationsTest, UpdateWithDrugEffectsNullDrugsThrows) {
  EXPECT_THROW(populations->update_with_drug_effects(nullptr), std::invalid_argument);
}

TEST_F(SingleHostClonalParasitePopulationsTest,
       UpdateWithDrugEffectsAndClearCuredMatchesSeparatePassesWithoutDrugs) {
  test_fixtures::setup_test_environment("test_input.yml");
  Model::get_instance()->release();
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  Model::get_instance()->initialize();

  struct Cleanup {
    ~Cleanup() {
      Model::get_instance()->release();
      test_fixtures::cleanup_test_files();
    }
  } cleanup;

  const auto &parasite_info = Model::get_config()
                                  ->get_genotype_parameters()
                                  .get_initial_parasite_info_raw()[0]
                                  .get_parasite_info()[0];
  auto* real_genotype = Model::get_genotype_db()->get_genotype(parasite_info.get_aa_sequence());
  auto separate_passes = std::make_unique<SingleHostClonalParasitePopulations>(person.get());
  auto combined_pass = std::make_unique<SingleHostClonalParasitePopulations>(person.get());
  auto drugs_in_blood = std::make_unique<DrugsInBlood>();

  for (auto* target : {separate_passes.get(), combined_pass.get()}) {
    auto parasite1 = std::make_unique<ClonalParasitePopulation>(real_genotype);
    parasite1->set_last_update_log10_parasite_density(5.0);
    parasite1->set_gametocyte_level(1.0);
    target->add(std::move(parasite1));

    auto parasite2 = std::make_unique<ClonalParasitePopulation>(real_genotype);
    parasite2->set_last_update_log10_parasite_density(
        Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
    parasite2->set_gametocyte_level(1.0);
    target->add(std::move(parasite2));

    auto parasite3 = std::make_unique<ClonalParasitePopulation>(real_genotype);
    parasite3->set_last_update_log10_parasite_density(3.0);
    parasite3->set_gametocyte_level(1.0);
    target->add(std::move(parasite3));
  }

  separate_passes->update_with_drug_effects(drugs_in_blood.get());
  separate_passes->clear_cured_parasites(Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
  combined_pass->update_with_drug_effects_and_clear_cured(
      drugs_in_blood.get(), Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);

  EXPECT_EQ(combined_pass->size(), separate_passes->size());
  EXPECT_DOUBLE_EQ(combined_pass->log10_total_infectious_density(),
                   separate_passes->log10_total_infectious_density());
}

TEST_F(SingleHostClonalParasitePopulationsTest, Clear) {
  auto parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());

  populations->add(std::move(parasite));
  EXPECT_EQ(populations->size(), 1);

  populations->clear();
  EXPECT_EQ(populations->size(), 0);
  EXPECT_TRUE(populations->empty());

  // Test clearing already empty population
  populations->clear();
  EXPECT_EQ(populations->size(), 0);
  EXPECT_TRUE(populations->empty());
}

TEST_F(SingleHostClonalParasitePopulationsTest, LatestUpdateTime) {
  // Set up the expectation before calling the method
  EXPECT_CALL(*person, get_latest_update_time()).WillOnce(testing::Return(10));

  // Call the method and verify the result
  int result = populations->latest_update_time();
  EXPECT_EQ(result, 10);
}

TEST_F(SingleHostClonalParasitePopulationsTest, IteratorAccess) {
  auto parasite1 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite2 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite1_ptr = parasite1.get();
  auto parasite2_ptr = parasite2.get();

  populations->add(std::move(parasite1));
  populations->add(std::move(parasite2));

  // Test iterator access
  auto it = populations->begin();
  EXPECT_EQ((*it).get(), parasite1_ptr);
  ++it;
  EXPECT_EQ((*it).get(), parasite2_ptr);

  // Test const iterator access
  const auto* const_populations = populations.get();
  auto const_it = const_populations->begin();
  EXPECT_EQ((*const_it).get(), parasite1_ptr);
  ++const_it;
  EXPECT_EQ((*const_it).get(), parasite2_ptr);
}

TEST_F(SingleHostClonalParasitePopulationsTest, IteratorEdgeCases) {
  // Test empty population iterators
  EXPECT_EQ(populations->begin(), populations->end());

  // Test iterator increment beyond end
  auto parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());
  populations->add(std::move(parasite));
  auto it = populations->begin();
  ++it;
  EXPECT_EQ(it, populations->end());
}

TEST_F(SingleHostClonalParasitePopulationsTest, MultipleParasitesWithDifferentDensities) {
  auto parasite1 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite2 = std::make_unique<ClonalParasitePopulation>(genotype.get());
  auto parasite3 = std::make_unique<ClonalParasitePopulation>(genotype.get());

  parasite1->set_last_update_log10_parasite_density(5.0);
  parasite1->set_gametocyte_level(1.0);
  parasite2->set_last_update_log10_parasite_density(3.0);
  parasite2->set_gametocyte_level(1.0);
  parasite3->set_last_update_log10_parasite_density(4.0);
  parasite3->set_gametocyte_level(1.0);

  populations->add(std::move(parasite1));
  populations->add(std::move(parasite2));
  populations->add(std::move(parasite3));

  // Test total infectious density calculation
  populations->clear_cured_parasites(Constants::DEFAULT_LOG10_PARASITE_DENSITY_CURED);
  EXPECT_GT(populations->log10_total_infectious_density(), 5.0);
}
