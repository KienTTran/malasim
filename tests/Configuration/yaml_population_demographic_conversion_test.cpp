#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Configuration/PopulationDemographic.h"

// Test Fixture Class
class PopulationDemographicTest : public ::testing::Test {
protected:
  PopulationDemographic default_demographic;

  void SetUp() override {
    // Initialize default PopulationDemographic object using setters
    default_demographic.set_age_structure({100, 150, 200, 150, 100});
    default_demographic.set_initial_age_structure({100, 150, 200, 150, 100});
    default_demographic.set_birth_rate(0.02);
    default_demographic.set_death_rate_by_age_class(
        {0.01, 0.015, 0.02, 0.015, 0.01});
    default_demographic.set_mortality_when_treatment_fail_by_age_class(
        {0.05, 0.07, 0.1, 0.07, 0.05});
    default_demographic.set_artificial_rescaling_of_population_size(1.0);
  }
};

// Test encoding functionality
TEST_F(PopulationDemographicTest, EncodePopulationDemographic) {
  YAML::Node node =
      YAML::convert<PopulationDemographic>::encode(default_demographic);

  EXPECT_EQ(node["age_structure"].as<std::vector<int>>(),
            default_demographic.get_age_structure());
  EXPECT_EQ(node["initial_age_structure"].as<std::vector<int>>(),
            default_demographic.get_initial_age_structure());
  EXPECT_DOUBLE_EQ(node["birth_rate"].as<double>(),
                   default_demographic.get_birth_rate());
  EXPECT_EQ(node["death_rate_by_age_class"].as<std::vector<double>>(),
            default_demographic.get_death_rate_by_age_class());
  EXPECT_EQ(
      node["mortality_when_treatment_fail_by_age_class"]
          .as<std::vector<double>>(),
      default_demographic.get_mortality_when_treatment_fail_by_age_class());
  EXPECT_DOUBLE_EQ(
      node["artificial_rescaling_of_population_size"].as<double>(),
      default_demographic.get_artificial_rescaling_of_population_size());
}

// Test decoding functionality
TEST_F(PopulationDemographicTest, DecodePopulationDemographic) {
  YAML::Node node;
  node["age_structure"] = std::vector<int>{100, 150, 200, 150, 100};
  node["initial_age_structure"] = std::vector<int>{100, 150, 200, 150, 100};
  node["birth_rate"] = 0.02;
  node["death_rate_by_age_class"] =
      std::vector<double>{0.01, 0.015, 0.02, 0.015, 0.01};
  node["mortality_when_treatment_fail_by_age_class"] =
      std::vector<double>{0.05, 0.07, 0.1, 0.07, 0.05};
  node["artificial_rescaling_of_population_size"] = 1.0;

  PopulationDemographic decoded_demographic;
  EXPECT_NO_THROW(
      YAML::convert<PopulationDemographic>::decode(node, decoded_demographic));

  EXPECT_EQ(decoded_demographic.get_number_of_age_classes(), 5);
  EXPECT_EQ(decoded_demographic.get_age_structure(),
            std::vector<int>({100, 150, 200, 150, 100}));
  EXPECT_EQ(decoded_demographic.get_initial_age_structure(),
            std::vector<int>({100, 150, 200, 150, 100}));
  EXPECT_DOUBLE_EQ(decoded_demographic.get_birth_rate(), 0.02);
  EXPECT_EQ(decoded_demographic.get_death_rate_by_age_class(),
            std::vector<double>({0.01, 0.015, 0.02, 0.015, 0.01}));
  EXPECT_EQ(
      decoded_demographic.get_mortality_when_treatment_fail_by_age_class(),
      std::vector<double>({0.05, 0.07, 0.1, 0.07, 0.05}));
  EXPECT_DOUBLE_EQ(
      decoded_demographic.get_artificial_rescaling_of_population_size(), 1.0);
}

// Test decoding with missing fields
TEST_F(PopulationDemographicTest, DecodePopulationDemographicMissingField) {
  YAML::Node node;
  node["number_of_age_classes"] = 5;
  // Intentionally omit other fields to trigger exceptions

  PopulationDemographic decoded_demographic;
  EXPECT_THROW(
      YAML::convert<PopulationDemographic>::decode(node, decoded_demographic),
      std::runtime_error);
}

// Test decoding with partial missing fields
TEST_F(PopulationDemographicTest,
       DecodePopulationDemographicPartialMissingFields) {
  YAML::Node node;
  node["number_of_age_classes"] = 5;
  node["age_structure"] = std::vector<int>{100, 150, 200, 150, 100};
  // Missing 'initial_age_structure' and other fields

  PopulationDemographic decoded_demographic;
  EXPECT_THROW(
      YAML::convert<PopulationDemographic>::decode(node, decoded_demographic),
      std::runtime_error);
}

// Test encoding and then decoding to ensure consistency
TEST_F(PopulationDemographicTest, EncodeDecodeConsistency) {
  YAML::Node node =
      YAML::convert<PopulationDemographic>::encode(default_demographic);

  PopulationDemographic decoded_demographic;
  EXPECT_NO_THROW(
      YAML::convert<PopulationDemographic>::decode(node, decoded_demographic));

  EXPECT_EQ(decoded_demographic.get_number_of_age_classes(),
            default_demographic.get_number_of_age_classes());
  EXPECT_EQ(decoded_demographic.get_age_structure(),
            default_demographic.get_age_structure());
  EXPECT_EQ(decoded_demographic.get_initial_age_structure(),
            default_demographic.get_initial_age_structure());
  EXPECT_DOUBLE_EQ(decoded_demographic.get_birth_rate(),
                   default_demographic.get_birth_rate());
  EXPECT_EQ(decoded_demographic.get_death_rate_by_age_class(),
            default_demographic.get_death_rate_by_age_class());
  EXPECT_EQ(
      decoded_demographic.get_mortality_when_treatment_fail_by_age_class(),
      default_demographic.get_mortality_when_treatment_fail_by_age_class());
  EXPECT_DOUBLE_EQ(
      decoded_demographic.get_artificial_rescaling_of_population_size(),
      default_demographic.get_artificial_rescaling_of_population_size());
}

TEST_F(PopulationDemographicTest, SetterValidationAndProcessConfig) {
  PopulationDemographic demographic;
  EXPECT_THROW(demographic.set_age_structure({}), std::invalid_argument);
  demographic.set_age_structure({10, 20});
  EXPECT_EQ(demographic.get_number_of_age_classes(), 2);
  EXPECT_THROW(demographic.set_initial_age_structure({10}), std::invalid_argument);
  EXPECT_NO_THROW(demographic.set_initial_age_structure({10, 20}));
  EXPECT_THROW(demographic.set_birth_rate(-0.1), std::invalid_argument);
  EXPECT_THROW(demographic.set_death_rate_by_age_class({0.1}), std::invalid_argument);
  EXPECT_NO_THROW(demographic.set_death_rate_by_age_class({0.1, 0.2}));
  EXPECT_THROW(demographic.set_mortality_when_treatment_fail_by_age_class({0.1}),
               std::invalid_argument);
  EXPECT_THROW(demographic.set_mortality_when_treatment_fail_by_age_class({0.1, -0.2}),
               std::invalid_argument);
  EXPECT_THROW(demographic.set_artificial_rescaling_of_population_size(0.0),
               std::invalid_argument);
  demographic.process_config();
  EXPECT_EQ(demographic.get_number_of_age_classes(), 2);
}
