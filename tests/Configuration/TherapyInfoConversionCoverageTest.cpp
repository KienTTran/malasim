#include <gtest/gtest.h>

#include "Configuration/TherapyParameters.h"

TEST(TherapyInfoConversionCoverageTest, EncodesAndDecodesAllTherapyInfoVariants) {
  TherapyParameters::TherapyInfo info;
  info.set_name("combination");
  info.set_drug_ids({1, 2});
  info.set_dosing_days({3, 5});
  info.set_therapy_ids({7, 8});
  info.set_regiment({10, 11});

  const auto encoded = YAML::convert<TherapyParameters::TherapyInfo>::encode(info);
  ASSERT_TRUE(encoded.IsMap());
  EXPECT_EQ(encoded["name"].as<std::string>(), "combination");
  EXPECT_EQ(encoded["drug_ids"].as<std::vector<int>>(), std::vector<int>({1, 2}));
  EXPECT_EQ(encoded["dosing_days"].as<std::vector<int>>(), std::vector<int>({3, 5}));
  EXPECT_EQ(encoded["therapy_ids"].as<std::vector<int>>(), std::vector<int>({7, 8}));
  EXPECT_EQ(encoded["regiment"].as<std::vector<int>>(), std::vector<int>({10, 11}));

  TherapyParameters::TherapyInfo decoded;
  ASSERT_TRUE(YAML::convert<TherapyParameters::TherapyInfo>::decode(encoded, decoded));
  EXPECT_EQ(decoded.get_name(), info.get_name());
  EXPECT_EQ(decoded.get_drug_ids(), info.get_drug_ids());
  EXPECT_EQ(decoded.get_dosing_days(), info.get_dosing_days());
  EXPECT_EQ(decoded.get_therapy_ids(), info.get_therapy_ids());
  EXPECT_EQ(decoded.get_regiment(), info.get_regiment());
}

TEST(TherapyInfoConversionCoverageTest, RejectsEmptyInfoAndAcceptsEachOptionalField) {
  TherapyParameters::TherapyInfo decoded;
  EXPECT_THROW(YAML::convert<TherapyParameters::TherapyInfo>::decode(YAML::Node(), decoded),
               std::runtime_error);

  for (const auto &yaml : {YAML::Load("drug_ids: [1]"), YAML::Load("dosing_days: [2]"),
                           YAML::Load("therapy_ids: [3]"), YAML::Load("regiment: [4]"),
                           YAML::Load("name: mono")}) {
    TherapyParameters::TherapyInfo value;
    EXPECT_TRUE(YAML::convert<TherapyParameters::TherapyInfo>::decode(yaml, value));
  }
}

TEST(TherapyInfoConversionCoverageTest, CoversTherapyParametersAccessorsAndLegacyEncoder) {
  TherapyParameters parameters;
  parameters.set_tf_testing_day(42);
  parameters.set_tf_rate(0.25);
  TherapyParameters::TherapyInfo info;
  info.set_name("legacy");
  info.set_drug_ids({4});
  info.set_dosing_days({1, 2});
  info.set_therapy_ids({9});
  info.set_regiment({3});
  parameters.set_therapy_db_raw({{2, info}});
  parameters.set_node(YAML::Load("{2: {drug_ids: [4]}}"));

  EXPECT_EQ(parameters.get_tf_testing_day(), 42);
  EXPECT_DOUBLE_EQ(parameters.get_tf_rate(), 0.25);
  ASSERT_EQ(parameters.get_therapy_db_raw().size(), 1U);
  EXPECT_EQ(parameters.get_therapy_db_raw().at(2).get_name(), "legacy");
  EXPECT_TRUE(parameters.get_node());

  const auto encoded = YAML::convert<TherapyParameters>::encode(info);
  EXPECT_EQ(encoded["name"].as<std::string>(), "legacy");
  EXPECT_EQ(encoded["drug_ids"].as<std::vector<int>>(), std::vector<int>{4});
  EXPECT_EQ(encoded["dosing_days"].as<std::vector<int>>(), std::vector<int>({1, 2}));
  EXPECT_EQ(encoded["therapy_ids"].as<std::vector<int>>(), std::vector<int>{9});
  EXPECT_EQ(encoded["regiment"].as<std::vector<int>>(), std::vector<int>{3});
}
