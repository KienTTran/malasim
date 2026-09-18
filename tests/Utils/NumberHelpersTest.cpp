#include <gtest/gtest.h>

#include "Utils/Helpers/NumberHelpers.h"

TEST(NumberHelpersTest, EvaluatesFloatingPointPredicates) {
  EXPECT_TRUE(NumberHelpers::is_equal(1.0, 1.0));
  EXPECT_FALSE(NumberHelpers::is_equal(1.0, 2.0));
  EXPECT_TRUE(NumberHelpers::is_not_equal(1.0, 2.0));
  EXPECT_TRUE(NumberHelpers::is_zero(0.0));
  EXPECT_FALSE(NumberHelpers::is_zero(1.0));
  EXPECT_TRUE(NumberHelpers::is_not_zero(1.0));
  EXPECT_FALSE(NumberHelpers::is_not_zero(0.0));
  EXPECT_TRUE(NumberHelpers::is_enot_qual(1.0, 2.0));
}

TEST(NumberHelpersTest, SupportsFloatAndIntegralPredicateInstantiations) {
  EXPECT_TRUE(NumberHelpers::is_equal<float>(1.0F, 1.0F));
  EXPECT_TRUE(NumberHelpers::is_not_equal<float>(1.0F, 2.0F));
  EXPECT_TRUE(NumberHelpers::is_zero<float>(0.0F));
  EXPECT_TRUE(NumberHelpers::is_not_zero<float>(1.0F));
  EXPECT_TRUE(NumberHelpers::is_enot_qual<float>(1.0F, 2.0F));

  EXPECT_TRUE(NumberHelpers::is_equal<int>(1, 1, 1));
  EXPECT_TRUE(NumberHelpers::is_not_equal<int>(1, 2, 1));
  EXPECT_TRUE(NumberHelpers::is_zero<int>(0, 1));
  EXPECT_TRUE(NumberHelpers::is_not_zero<int>(1, 1));
  EXPECT_TRUE(NumberHelpers::is_enot_qual<int>(1, 2, 1));
}

TEST(NumberHelpersTest, ConvertsNumbersAndGeneratesSeed) {
  EXPECT_EQ(NumberHelpers::number_to_string(42), "42");
  EXPECT_EQ(NumberHelpers::single_digit_number_to_char(7), '7');
  EXPECT_EQ(NumberHelpers::char_to_single_digit_number('7'), 7);
  EXPECT_NO_THROW({
    const auto seed = NumberHelpers::good_seed(1, 2, 3);
    (void)seed;
  });
}
