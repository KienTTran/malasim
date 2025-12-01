#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "Configuration/TherapyParameters.h"

class TherapyParametersTest : public ::testing::Test {
protected:
    TherapyParameters therapy_parameters;

    void SetUp() override {
        // Set up the therapy information for testing
        TherapyParameters::TherapyInfo therapy_info_mono_as;
        therapy_info_mono_as.set_drug_ids({0});
        therapy_info_mono_as.set_dosing_days({3});

        TherapyParameters::TherapyInfo therapy_info_mono_lumefaltrine;
        therapy_info_mono_lumefaltrine.set_drug_ids({1});
        therapy_info_mono_lumefaltrine.set_dosing_days({3});

        // Create the map for therapy_db
        std::map<int, TherapyParameters::TherapyInfo> therapy_db;
        therapy_db[0] = therapy_info_mono_as;
        therapy_db[1] = therapy_info_mono_lumefaltrine;

        therapy_parameters.set_tf_testing_day(28);
        therapy_parameters.set_tf_rate(0.1);
        therapy_parameters.set_therapy_db_raw(therapy_db);
    }
};

// TEST_F(TherapyParametersTest, EncodeTherapyParameters) {
//     YAML::Node node = YAML::convert<TherapyParameters>::encode(therapy_parameters);
//
//     EXPECT_EQ(node["tf_testing_day"].as<int>(), 28);
//     EXPECT_EQ(node["tf_rate"].as<double>(), 0.1);
//     EXPECT_EQ(node["therapy_db"]["0"]["drug_ids"].as<std::vector<int>>(), std::vector<int>{0});
//     EXPECT_EQ(node["therapy_db"]["0"]["dosing_days"].as<std::vector<int>>(), std::vector<int>{3});
//     EXPECT_EQ(node["therapy_db"]["1"]["drug_ids"].as<std::vector<int>>(), std::vector<int>{1});
//     EXPECT_EQ(node["therapy_db"]["1"]["dosing_days"].as<std::vector<int>>(), std::vector<int>{3});
// }

TEST_F(TherapyParametersTest, DecodeTherapyParameters) {
    YAML::Node node;
    node["tf_testing_day"] = 28;
    node["tf_rate"] = 0.1;

    node["therapy_db"]["0"]["drug_ids"] = std::vector<int>{0};
    node["therapy_db"]["0"]["dosing_days"] = std::vector<int>{3};

    node["therapy_db"]["1"]["drug_ids"] = std::vector<int>{1};
    node["therapy_db"]["1"]["dosing_days"] = std::vector<int>{3};

    // Add more entries as needed

    TherapyParameters decoded_parameters;
    EXPECT_NO_THROW(YAML::convert<TherapyParameters>::decode(node, decoded_parameters));

    EXPECT_EQ(decoded_parameters.get_tf_testing_day(), 28);
    EXPECT_EQ(decoded_parameters.get_tf_rate(), 0.1);
    EXPECT_EQ(decoded_parameters.get_therapy_db_raw().at(0).get_drug_ids(), std::vector<int>{0});
    EXPECT_EQ(decoded_parameters.get_therapy_db_raw().at(0).get_dosing_days(), std::vector<int>{3});
    EXPECT_EQ(decoded_parameters.get_therapy_db_raw().at(1).get_drug_ids(), std::vector<int>{1});
    EXPECT_EQ(decoded_parameters.get_therapy_db_raw().at(1).get_dosing_days(), std::vector<int>{3});
}

// Test for missing fields in the input YAML
TEST_F(TherapyParametersTest, DecodeTherapyParametersMissingField) {
    YAML::Node node;
    node["tf_testing_day"] = 28;  // Missing other fields

    TherapyParameters decoded_parameters;
    EXPECT_THROW(YAML::convert<TherapyParameters>::decode(node, decoded_parameters), std::runtime_error);
}
