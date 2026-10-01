#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "RandomTestBase.h"

TEST_F(RandomTest, SamplingHelpersHandleEmptyAndZeroWeightInputs) {
  std::vector<int*> empty_objects;
  std::vector<double> empty_distribution;
  const auto empty = rng.roulette_sampling<int>(3, empty_distribution, empty_objects, false);
  ASSERT_EQ(empty.size(), 3U);
  EXPECT_EQ(empty[0], nullptr);

  int first = 1;
  std::vector<int*> objects{&first};
  std::vector<double> zero_distribution{0.0};
  const auto multinomial =
      rng.multinomial_sampling<int>(2, zero_distribution, objects, false, -1.0);
  ASSERT_EQ(multinomial.size(), 2U);
  EXPECT_EQ(multinomial[1], nullptr);

  const auto tuples = rng.roulette_sampling_tuple<int>(2, zero_distribution, objects, false, 0.0);
  ASSERT_EQ(tuples.size(), 2U);
  EXPECT_EQ(std::get<0>(tuples[0]), nullptr);
  EXPECT_DOUBLE_EQ(std::get<1>(tuples[0]), 0.0);
}

TEST_F(RandomTest, SamplingTupleReturnsWeightedObject) {
  int first = 1;
  std::vector<int*> objects{&first};
  std::vector<double> distribution{1.0};
  const auto tuples = rng.roulette_sampling_tuple<int>(4, distribution, objects, true, 1.0);
  ASSERT_EQ(tuples.size(), 4U);
  for (const auto &sample : tuples) {
    EXPECT_EQ(std::get<0>(sample), &first);
    EXPECT_DOUBLE_EQ(std::get<1>(sample), 1.0);
  }
}

class FixedRouletteRandom : public utils::Random {
public:
  double random_uniform() override { return 0.75; }
};

TEST(RandomRouletteEdgeTest, PrecomputedTotalCannotChangeTheDistribution) {
  FixedRouletteRandom fixed_rng;
  int first = 1;
  int second = 2;
  std::vector<int*> objects{&first, &second};
  std::vector<double> distribution{1.0, 1.0};

  const auto too_large = fixed_rng.roulette_sampling<int>(1, distribution, objects, false, 4.0);
  ASSERT_EQ(too_large.size(), 1U);
  EXPECT_EQ(too_large[0], &second);

  const auto too_small = fixed_rng.roulette_sampling<int>(1, distribution, objects, false, 1.0);
  ASSERT_EQ(too_small.size(), 1U);
  EXPECT_EQ(too_small[0], &second);

  const auto tuple = fixed_rng.roulette_sampling_tuple<int>(1, distribution, objects, false, 4.0);
  ASSERT_EQ(tuple.size(), 1U);
  EXPECT_EQ(std::get<0>(tuple[0]), &second);
  EXPECT_DOUBLE_EQ(std::get<1>(tuple[0]), 1.0);
}

TEST(RandomRouletteEdgeTest, MismatchedVectorsAreRejected) {
  FixedRouletteRandom fixed_rng;
  int first = 1;
  std::vector<int*> objects{&first};
  std::vector<double> distribution{0.0, 1.0};

  EXPECT_THROW(static_cast<void>(fixed_rng.roulette_sampling<int>(1, distribution, objects, false)),
               std::invalid_argument);
  EXPECT_THROW(
      static_cast<void>(fixed_rng.roulette_sampling_tuple<int>(1, distribution, objects, false)),
      std::invalid_argument);
}

TEST(RandomRouletteEdgeTest, NegativeAndNonFiniteWeightsAreRejected) {
  FixedRouletteRandom fixed_rng;
  int first = 1;
  std::vector<int*> objects{&first};
  std::vector<double> distribution{-1.0};
  EXPECT_THROW(static_cast<void>(fixed_rng.roulette_sampling<int>(1, distribution, objects, false)),
               std::invalid_argument);

  distribution[0] = std::numeric_limits<double>::quiet_NaN();
  EXPECT_THROW(
      static_cast<void>(fixed_rng.roulette_sampling_tuple<int>(1, distribution, objects, false)),
      std::invalid_argument);
}

TEST(RandomRouletteEdgeTest, PositiveSubnormalWeightsStillSelectAnObject) {
  FixedRouletteRandom fixed_rng;
  int first = 1;
  int second = 2;
  std::vector<int*> objects{&first, &second};
  const auto tiny_weight = std::numeric_limits<double>::denorm_min();
  std::vector<double> distribution{tiny_weight, tiny_weight};

  const auto samples = fixed_rng.roulette_sampling<int>(1, distribution, objects, false);
  ASSERT_EQ(samples.size(), 1U);
  EXPECT_NE(samples[0], nullptr);

  const auto tuples = fixed_rng.roulette_sampling_tuple<int>(1, distribution, objects, false);
  ASSERT_EQ(tuples.size(), 1U);
  EXPECT_NE(std::get<0>(tuples[0]), nullptr);
  EXPECT_DOUBLE_EQ(std::get<1>(tuples[0]), tiny_weight);
}
