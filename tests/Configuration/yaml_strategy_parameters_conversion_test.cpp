#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "Configuration/StrategyParameters.h"

// Helper function to compare two std::vector<std::vector<double>>
bool inline compareNestedVectors(const std::vector<std::vector<double>>& vec1, const std::vector<std::vector<double>>& vec2) {
  if (vec1.size() != vec2.size()) return false;
  for (size_t i = 0; i < vec1.size(); ++i) {
    if (vec1[i] != vec2[i]) return false;  // Compare inner vectors
  }
  return true;
}


class StrategyParametersTest : public ::testing::Test {
protected:
    StrategyParameters strategy_parameters;

    void SetUp() override {
        // Setup StrategyInfo
        StrategyParameters::StrategyInfo strategy_info;
        strategy_info.set_name("SP-AQ-CQ-AL-MFTStrategy");
        strategy_info.set_type("MFT");
        strategy_info.set_therapy_ids({5, 2, 12, 6});
        strategy_info.set_distribution({0.3, 0.3, 0.3, 0.1});

        // Set start_distribution_by_location and peak_distribution_by_location
        strategy_info.set_start_distribution_by_location({{0.1, 0.2}, {0.3, 0.4}});
        strategy_info.set_peak_distribution_by_location({{0.5, 0.6}, {0.7, 0.8}});

        // Setup MassDrugAdministration
        StrategyParameters::MassDrugAdministration mda_info;
        mda_info.set_enable(false);
        mda_info.set_mda_therapy_id(8);
        mda_info.set_age_bracket_prob_individual_present_at_mda({10, 40});
        mda_info.set_mean_prob_individual_present_at_mda({0.85, 0.75, 0.85});
        mda_info.set_sd_prob_individual_present_at_mda({0.3, 0.3, 0.3});

        // Set StrategyParameters with a map
        std::map<int, StrategyParameters::StrategyInfo> strategy_db;
        strategy_db[0] = strategy_info;  // Using integer key 0 for the map

        strategy_parameters.set_strategy_db_raw(strategy_db);
        strategy_parameters.set_initial_strategy_id(15);
        strategy_parameters.set_mass_drug_administration(mda_info);
    }
};

// Test encoding functionality
TEST_F(StrategyParametersTest, EncodeStrategyParameters) {
    YAML::Node node = YAML::convert<StrategyParameters>::encode(strategy_parameters);

    EXPECT_EQ(node["initial_strategy_id"].as<int>(), 15);
    EXPECT_EQ(node["strategy_db"][0]["name"].as<std::string>(), "SP-AQ-CQ-AL-MFTStrategy");
    EXPECT_EQ(node["strategy_db"][0]["therapy_ids"].as<std::vector<int>>(), std::vector<int>({5, 2, 12, 6}));

    // Check the new start_distribution_by_location and peak_distribution_by_location fields
    auto start_distribution = node["strategy_db"][0]["start_distribution_by_location"].as<std::vector<std::vector<double>>>();
    auto peak_distribution = node["strategy_db"][0]["peak_distribution_by_location"].as<std::vector<std::vector<double>>>();
    EXPECT_TRUE(compareNestedVectors(start_distribution, {{0.1, 0.2}, {0.3, 0.4}}));
    EXPECT_TRUE(compareNestedVectors(peak_distribution, {{0.5, 0.6}, {0.7, 0.8}}));

    EXPECT_EQ(node["mass_drug_administration"]["enable"].as<bool>(), false);
    EXPECT_EQ(node["mass_drug_administration"]["age_bracket_prob_individual_present_at_mda"].as<std::vector<int>>(), std::vector<int>({10, 40}));
    EXPECT_EQ(node["mass_drug_administration"]["mean_prob_individual_present_at_mda"].as<std::vector<double>>(), std::vector<double>({0.85, 0.75, 0.85}));
}

// Test decoding functionality
TEST_F(StrategyParametersTest, DecodeStrategyParameters) {
    YAML::Node node;
    node["initial_strategy_id"] = 15;

    node["strategy_db"]["0"]["name"] = "SP-AQ-CQ-AL-MFTStrategy";
    node["strategy_db"]["0"]["type"] = "MFT";
    node["strategy_db"]["0"]["therapy_ids"] = std::vector<int>{5, 2, 12, 6};
    node["strategy_db"]["0"]["distribution"] = std::vector<double>{0.3, 0.3, 0.3, 0.1};

    // Set the new start_distribution_by_location and peak_distribution_by_location fields
    node["strategy_db"]["0"]["start_distribution_by_location"] = std::vector<std::vector<double>>{{0.1, 0.2}, {0.3, 0.4}};
    node["strategy_db"]["0"]["peak_distribution_by_location"] = std::vector<std::vector<double>>{{0.5, 0.6}, {0.7, 0.8}};

    node["mass_drug_administration"]["enable"] = false;
    node["mass_drug_administration"]["mda_therapy_id"] = 8;
    node["mass_drug_administration"]["age_bracket_prob_individual_present_at_mda"] = std::vector<int>{10, 40};
    node["mass_drug_administration"]["mean_prob_individual_present_at_mda"] = std::vector<double>{0.85, 0.75, 0.85};
    node["mass_drug_administration"]["sd_prob_individual_present_at_mda"] = std::vector<double>{0.3, 0.3, 0.3};

    StrategyParameters decoded_parameters;
    EXPECT_NO_THROW(YAML::convert<StrategyParameters>::decode(node, decoded_parameters));

    EXPECT_EQ(decoded_parameters.get_initial_strategy_id(), 15);
    EXPECT_EQ(decoded_parameters.get_strategy_db_raw().at(0).get_name(), "SP-AQ-CQ-AL-MFTStrategy");
    EXPECT_EQ(decoded_parameters.get_strategy_db_raw().at(0).get_therapy_ids(), std::vector<int>({5, 2, 12, 6}));

    // Check the decoded values for the new fields
    EXPECT_TRUE(compareNestedVectors(decoded_parameters.get_strategy_db_raw().at(0).get_start_distribution_by_location(),
                                     {{0.1, 0.2}, {0.3, 0.4}}));
    EXPECT_TRUE(compareNestedVectors(decoded_parameters.get_strategy_db_raw().at(0).get_peak_distribution_by_location(),
                                     {{0.5, 0.6}, {0.7, 0.8}}));

    EXPECT_EQ(decoded_parameters.get_mda().get_enable(), false);
    EXPECT_EQ(decoded_parameters.get_mda().get_age_bracket_prob_individual_present_at_mda(), std::vector<int>({10, 40}));
    EXPECT_EQ(decoded_parameters.get_mda().get_mean_prob_individual_present_at_mda(), std::vector<double>({0.85, 0.75, 0.85}));
}

// Test for decoding with missing fields
TEST_F(StrategyParametersTest, DecodeStrategyParametersMissingField) {
    YAML::Node node;
    node["initial_strategy_id"] = 15;  // Missing other fields

    StrategyParameters decoded_parameters;
    EXPECT_THROW(YAML::convert<StrategyParameters>::decode(node, decoded_parameters), std::runtime_error);
}
