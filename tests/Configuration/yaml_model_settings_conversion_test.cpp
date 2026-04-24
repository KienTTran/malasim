#include <date/date.h>
#include <gtest/gtest.h>

#include "Configuration/ModelSettings.h"
#include "Configuration/Config.h"
#include "Configuration/SeasonalitySettings.h"

class ModelSettingsTest : public ::testing::Test {
protected:
  ModelSettings default_settings;

  void SetUp() override {
    // Initialize default ModelSettings object using setters
    default_settings.set_days_between_stdout_output(10);
    default_settings.set_initial_seed_number(123);
    default_settings.set_record_genome_db(true);
  }
};

// Test encoding functionality
TEST_F(ModelSettingsTest, EncodeModelSettings) {
  default_settings.set_minimum_days_for_counting_new_clinical_episode(5);
  YAML::Node node = YAML::convert<ModelSettings>::encode(default_settings);

  EXPECT_EQ(node["days_between_stdout_output"].as<int>(),
            default_settings.get_days_between_stdout_output());
  EXPECT_EQ(node["initial_seed_number"].as<int>(),
            default_settings.get_initial_seed_number());
  EXPECT_EQ(node["record_genome_db"].as<bool>(),
  default_settings.get_record_genome_db());
  EXPECT_EQ(node["cell_level_reporting"].as<bool>(),
            default_settings.get_cell_level_reporting());
  EXPECT_EQ(node["minimum_days_for_counting_new_clinical_episode"].as<int>(),
            default_settings.get_minimum_days_for_counting_new_clinical_episode());
}

// Test decoding functionality
TEST_F(ModelSettingsTest, DecodeModelSettings) {
  YAML::Node node;
  node["days_between_stdout_output"] = 10;
  node["initial_seed_number"] = 123;
  node["record_genome_db"] = true;
  node["cell_level_reporting"] = true;
  node["minimum_days_for_counting_new_clinical_episode"] = 7;

  ModelSettings decoded_settings;
  EXPECT_NO_THROW(YAML::convert<ModelSettings>::decode(node, decoded_settings));

  EXPECT_EQ(decoded_settings.get_days_between_stdout_output(), 10);
  EXPECT_EQ(decoded_settings.get_initial_seed_number(), 123);
  EXPECT_EQ(decoded_settings.get_record_genome_db(), true);
  EXPECT_EQ(decoded_settings.get_cell_level_reporting(), true);
  EXPECT_EQ(decoded_settings.get_minimum_days_for_counting_new_clinical_episode(), 7);
}

// Test missing fields during decoding
TEST_F(ModelSettingsTest, DecodeModelSettingsMissingField) {
  YAML::Node node;
  node["initial_seed_number"] = 123;  // intentionally omit other fields

  ModelSettings decoded_settings;
  EXPECT_THROW(YAML::convert<ModelSettings>::decode(node, decoded_settings),
               std::runtime_error);
}
