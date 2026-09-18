// RandomTestBase.h

#ifndef RANDOM_TEST_BASE_H
#define RANDOM_TEST_BASE_H

#include <gtest/gtest.h>

#include "Utils/Random.h"

// Test fixture for Random class
class RandomTest : public ::testing::Test {
protected:
  utils::Random rng;
  void SetUp() override {
    // Keep statistical assertions reproducible across full-suite and coverage runs.
    rng.set_seed(42);
  }

  void TearDown() override {
    // Optional: Clean up any shared resources
  }
};

#endif  // RANDOM_TEST_BASE_H
