#include <gtest/gtest.h>

#include "Utils/Helpers/TimeHelpers.h"

TEST(TimeHelpersTest, CalculatesCalendarDifferences) {
  const auto start = date::sys_days{date::year{2024} / 1 / 1};
  const auto end = date::sys_days{date::year{2024} / 2 / 1};

  EXPECT_EQ(TimeHelpers::number_of_days(start, end), 31);
  EXPECT_EQ(TimeHelpers::days_between(date::year{2024} / 1 / 1,
                                      date::year{2024} / 2 / 1), 31);
  EXPECT_EQ(TimeHelpers::month_of_year(end), 2U);
  EXPECT_EQ(TimeHelpers::day_of_year(end), 32);
  EXPECT_EQ(TimeHelpers::number_of_days_to_next_year(start), 366);
}

TEST(TimeHelpersTest, HandlesLeapYearsAndMonthLengths) {
  EXPECT_TRUE(TimeHelpers::is_leap_year(2024));
  EXPECT_FALSE(TimeHelpers::is_leap_year(1900));
  EXPECT_TRUE(TimeHelpers::is_leap_year(2000));
  EXPECT_EQ(TimeHelpers::days_in_month(2024, 2), 29U);
  EXPECT_EQ(TimeHelpers::days_in_month(2023, 2), 28U);
  EXPECT_EQ(TimeHelpers::days_in_month(2024, 4), 30U);
  EXPECT_EQ(TimeHelpers::days_in_month(2024, 1), 31U);
}

TEST(TimeHelpersTest, CalculatesSimulationBirthdayOffset) {
  const auto starting_day = date::sys_days{date::year{2024} / 6 / 1};
  EXPECT_LT(TimeHelpers::get_simulation_time_birthday(30, 5, starting_day), 0);
}
