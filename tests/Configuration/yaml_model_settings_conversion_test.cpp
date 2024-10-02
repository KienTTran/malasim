#include <date/date.h>
#include <gtest/gtest.h>

#include "Configuration/ModelSettings.h"
#include "Configuration/YAMLConverters.h"

class ModelSettingsTest : public ::testing::Test {
protected:
  ModelSettings default_settings;

  void SetUp() override {
    // Initialize default ModelSettings object
    default_settings.days_between_stdout_output = 10;
    default_settings.initial_seed_number = 123;
    default_settings.record_genome_db = true;
    default_settings.starting_date =
        date::year_month_day{date::year{2024}, date::month{10}, date::day{1}};
    default_settings.start_of_comparison_period =
        date::year_month_day{date::year{2024}, date::month{10}, date::day{1}};
    default_settings.ending_date =
        date::year_month_day{date::year{2024}, date::month{10}, date::day{2}};
    default_settings.start_collect_data_day = 1;
  }
};

// Test encoding functionality
TEST_F(ModelSettingsTest, EncodeModelSettings) {
  YAML::Node node = YAML::convert<ModelSettings>::encode(default_settings);

  EXPECT_EQ(node["days_between_stdout_output"].as<int>(),
            default_settings.days_between_stdout_output);
  EXPECT_EQ(node["initial_seed_number"].as<int>(),
            default_settings.initial_seed_number);
  EXPECT_EQ(node["record_genome_db"].as<bool>(),
            default_settings.record_genome_db);
  EXPECT_EQ(node["starting_date"].as<date::year_month_day>(),
            default_settings.starting_date);
  EXPECT_EQ(node["start_of_comparison_period"].as<date::year_month_day>(),
            default_settings.start_of_comparison_period);
  EXPECT_EQ(node["ending_date"].as<date::year_month_day>(),
            default_settings.ending_date);
  EXPECT_EQ(node["start_collect_data_day"].as<int>(),
            default_settings.start_collect_data_day);
}

// Test decoding functionality
TEST_F(ModelSettingsTest, DecodeModelSettings) {
  YAML::Node node;
  node["days_between_stdout_output"] = 10;
  node["initial_seed_number"] = 123;
  node["record_genome_db"] = true;
  node["starting_date"] =
      date::year_month_day{date::year{2024}, date::month{10}, date::day{1}};
  node["start_of_comparison_period"] =
      date::year_month_day{date::year{2024}, date::month{10}, date::day{1}};
  node["ending_date"] =
      date::year_month_day{date::year{2024}, date::month{10}, date::day{2}};
  node["start_collect_data_day"] = 1;

  ModelSettings decoded_settings;
  EXPECT_NO_THROW(YAML::convert<ModelSettings>::decode(node, decoded_settings));

  EXPECT_EQ(decoded_settings.days_between_stdout_output, 10);
  EXPECT_EQ(decoded_settings.initial_seed_number, 123);
  EXPECT_EQ(decoded_settings.record_genome_db, true);

  auto expected_starting_date =
      date::year_month_day{date::year{2024}, date::month{10}, date::day{1}};
  EXPECT_EQ(decoded_settings.starting_date, expected_starting_date);

  auto expected_start_of_comparison_period =
      date::year_month_day{date::year{2024}, date::month{10}, date::day{1}};
  EXPECT_EQ(decoded_settings.start_of_comparison_period,
            expected_start_of_comparison_period);

  auto expected_ending_date =
      date::year_month_day{date::year{2024}, date::month{10}, date::day{2}};
  EXPECT_EQ(decoded_settings.ending_date, expected_ending_date);

  EXPECT_EQ(decoded_settings.start_collect_data_day, 1);
}

// Test missing fields during decoding
TEST_F(ModelSettingsTest, DecodeModelSettingsMissingField) {
  YAML::Node node;
  node["initial_seed_number"] = 123;  // intentionally omit other fields

  ModelSettings decoded_settings;
  EXPECT_THROW(YAML::convert<ModelSettings>::decode(node, decoded_settings),
               std::runtime_error);
}

