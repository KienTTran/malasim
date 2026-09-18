#include <gtest/gtest.h>

#include "Configuration/GenotypeParameters.h"

TEST(GenotypeParameterConversionEdgeTest, ExercisesFormattingHelpers) {
  GenotypeParameters::MultiplicativeEffectOnEC50 effect;
  effect.set_drug_id(2);
  effect.set_factors({1.0, 2.0});
  EXPECT_NE(effect.to_string().find("2:"), std::string::npos);

  GenotypeParameters::AminoAcidPosition position;
  position.set_position(4);
  position.set_amino_acids({"A", "C"});
  position.set_daily_crs({0.1, 0.2});
  position.set_multiplicative_effect_on_EC50({effect});
  EXPECT_EQ(position.get_amino_acids_string(), "A C ");
  EXPECT_EQ(position.get_daily_crs_string(), "0.100000 0.200000 ");
  EXPECT_NE(position.get_multiplicative_effect_on_EC50_string().find("2:"), std::string::npos);
  EXPECT_NE(position.to_string().find("A"), std::string::npos);
}

TEST(GenotypeParameterConversionEdgeTest, RejectsMissingFieldsInNestedConversions) {
  GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations two_mutations;
  EXPECT_THROW(YAML::convert<GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations>::decode(
                   YAML::Node(), two_mutations),
               std::runtime_error);

  GenotypeParameters::MultiplicativeEffectOnEC50 effect;
  EXPECT_THROW(YAML::convert<GenotypeParameters::MultiplicativeEffectOnEC50>::decode(YAML::Node(), effect),
               std::runtime_error);

  GenotypeParameters::AminoAcidPosition position;
  EXPECT_THROW(YAML::convert<GenotypeParameters::AminoAcidPosition>::decode(YAML::Node(), position),
               std::runtime_error);

  GenotypeParameters::GeneInfo gene;
  EXPECT_THROW(YAML::convert<GenotypeParameters::GeneInfo>::decode(YAML::Node(), gene),
               std::runtime_error);

  GenotypeParameters::OverrideEC50Pattern override_pattern;
  EXPECT_THROW(YAML::convert<GenotypeParameters::OverrideEC50Pattern>::decode(YAML::Node(), override_pattern),
               std::runtime_error);

  GenotypeParameters::ParasiteInfo parasite;
  EXPECT_THROW(YAML::convert<GenotypeParameters::ParasiteInfo>::decode(YAML::Node(), parasite),
               std::runtime_error);

  GenotypeParameters::InitialParasiteInfoRaw initial;
  EXPECT_THROW(YAML::convert<GenotypeParameters::InitialParasiteInfoRaw>::decode(YAML::Node(), initial),
               std::runtime_error);
}

TEST(GenotypeParameterConversionEdgeTest, RejectsUnsupportedMutationMaskNodeType) {
  YAML::Node node;
  node["mutation_mask"] = YAML::Node(YAML::NodeType::Map);
  node["mutation_probability_per_locus"] = 0.1;
  node["pf_genotype_info"] = YAML::Node(YAML::NodeType::Sequence);
  node["override_ec50_patterns"] = YAML::Node(YAML::NodeType::Sequence);
  node["initial_parasite_info"] = YAML::Node(YAML::NodeType::Sequence);
  GenotypeParameters parameters;
  EXPECT_THROW(YAML::convert<GenotypeParameters>::decode(node, parameters), std::runtime_error);
}
