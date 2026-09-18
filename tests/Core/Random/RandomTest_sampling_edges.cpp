#include <gtest/gtest.h>

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

  const auto tuples =
      rng.roulette_sampling_tuple<int>(2, zero_distribution, objects, false, 0.0);
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
