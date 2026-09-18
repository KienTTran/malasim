#include <gtest/gtest.h>

#include <memory>
#include <set>

#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"

class SingleHostClonalParasitePopulationsMemoryTest : public ::testing::Test {
 protected:
  class TrackedParasite : public ClonalParasitePopulation {
   public:
    explicit TrackedParasite(int id) : id_(id) {}
    ~TrackedParasite() override { destroyed_.insert(id_); }

   private:
    int id_;
  };

  void SetUp() override {
    person_ = std::make_unique<Person>();
    genotype_ = std::make_unique<Genotype>("abcdef");
    populations_ = std::make_unique<SingleHostClonalParasitePopulations>(person_.get());
    destroyed_.clear();
  }

  void TearDown() override {
    populations_.reset();
    person_.reset();
    genotype_.reset();
  }

  std::unique_ptr<TrackedParasite> tracked(int id) {
    auto parasite = std::make_unique<TrackedParasite>(id);
    parasite->set_last_update_log10_parasite_density(2.0);
    return parasite;
  }

  static bool destroyed(int id) { return destroyed_.contains(id); }

  std::unique_ptr<Person> person_;
  std::unique_ptr<Genotype> genotype_;
  std::unique_ptr<SingleHostClonalParasitePopulations> populations_;
  static std::set<int> destroyed_;
};

std::set<int> SingleHostClonalParasitePopulationsMemoryTest::destroyed_;

TEST_F(SingleHostClonalParasitePopulationsMemoryTest, UpdatesIndicesAfterRemovingMiddleParasite) {
  auto first = std::make_unique<ClonalParasitePopulation>(genotype_.get());
  auto middle = std::make_unique<ClonalParasitePopulation>(genotype_.get());
  auto last = std::make_unique<ClonalParasitePopulation>(genotype_.get());
  auto* first_ptr = first.get();
  auto* middle_ptr = middle.get();
  auto* last_ptr = last.get();
  populations_->add(std::move(first));
  populations_->add(std::move(middle));
  populations_->add(std::move(last));

  populations_->remove(middle_ptr->get_index());
  EXPECT_EQ(first_ptr->get_index(), 0U);
  EXPECT_EQ(last_ptr->get_index(), 1U);
  EXPECT_FALSE(populations_->contain(middle_ptr));
}

TEST_F(SingleHostClonalParasitePopulationsMemoryTest, ClearsCuredParasitesAndRetainsHealthyOnes) {
  auto healthy = tracked(1);
  auto cured = tracked(2);
  healthy->set_last_update_log10_parasite_density(2.0);
  cured->set_last_update_log10_parasite_density(-4.0);
  populations_->add(std::move(healthy));
  populations_->add(std::move(cured));

  populations_->clear_cured_parasites(-2.0);
  EXPECT_FALSE(destroyed(1));
  EXPECT_TRUE(destroyed(2));
  EXPECT_EQ(populations_->size(), 1U);
}

TEST_F(SingleHostClonalParasitePopulationsMemoryTest, ReclaimsAllParasitesOnClear) {
  populations_->add(tracked(3));
  populations_->add(tracked(4));
  populations_->clear();
  EXPECT_TRUE(destroyed(3));
  EXPECT_TRUE(destroyed(4));
  EXPECT_TRUE(populations_->empty());
}

TEST_F(SingleHostClonalParasitePopulationsMemoryTest, RetainsOwnershipUntilPopulationClear) {
  populations_->add(tracked(10));
  EXPECT_FALSE(destroyed(10));
  populations_->clear();
  EXPECT_TRUE(destroyed(10));
}

TEST_F(SingleHostClonalParasitePopulationsMemoryTest, DestroysRemovedParasiteOnly) {
  populations_->add(tracked(11));
  populations_->add(tracked(12));
  populations_->remove(0);
  EXPECT_TRUE(destroyed(11));
  EXPECT_FALSE(destroyed(12));
}

TEST_F(SingleHostClonalParasitePopulationsMemoryTest, DestroysAllParasitesWithOwner) {
  {
    auto local = std::make_unique<SingleHostClonalParasitePopulations>(person_.get());
    local->add(tracked(13));
    local->add(tracked(14));
  }
  EXPECT_TRUE(destroyed(13));
  EXPECT_TRUE(destroyed(14));
}

TEST_F(SingleHostClonalParasitePopulationsMemoryTest, ReclaimsMovedBackParasitesOnRepeatedRemoval) {
  for (int id = 0; id < 10; ++id) { populations_->add(tracked(id)); }
  for (int i = 0; i < 4; ++i) { populations_->remove(3); }

  EXPECT_EQ(populations_->size(), 6U);
  for (int id : {3, 7, 8, 9}) { EXPECT_TRUE(destroyed(id)); }
  for (int id : {0, 1, 2, 4, 5, 6}) { EXPECT_FALSE(destroyed(id)); }
}
