#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Configuration/GenotypeParameters.h"

namespace {
const std::vector<bool> kMutationMask{
    false, false, false, false, true,  true,  true,  false, false, true,
    false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, true, true, false, false, true};

YAML::Node make_mutation_mask_node() {
  YAML::Node node(YAML::NodeType::Sequence);
  for (const bool is_mutatable : kMutationMask) { node.push_back(is_mutatable); }
  node.SetStyle(YAML::EmitterStyle::Flow);
  return node;
}
}  // namespace

class GenotypeParametersTest : public ::testing::Test {
protected:
  GenotypeParameters genotype_parameters;

  void SetUp() override {
    // Set up multiplicative effect on EC50
    GenotypeParameters::MultiplicativeEffectOnEC50 multiplicative_effect;
    multiplicative_effect.set_drug_id(4);
    multiplicative_effect.set_factors({1.0, 2.44444444});

    // Set up amino acid positions
    GenotypeParameters::AminoAcidPosition aa_position;
    aa_position.set_position(86);
    aa_position.set_amino_acids({"N", "Y"});
    aa_position.set_daily_crs({0.0, 0.0005});
    aa_position.set_multiplicative_effect_on_EC50({multiplicative_effect});

    // Set up gene info
    GenotypeParameters::GeneInfo gene_info;
    gene_info.set_name("Pfmdr1");
    gene_info.set_max_copies(2);
    gene_info.set_cnv_reversion_multiplier(0.5);
    gene_info.set_cnv_daily_crs({0.0, 0.0005});
    gene_info.set_cnv_multiplicative_effect_on_EC50({multiplicative_effect});
    gene_info.set_aa_positions({aa_position});

    // Set up chromosome info
    GenotypeParameters::ChromosomeInfo chromosome_info;
    chromosome_info.set_chromosome_id(5);
    chromosome_info.set_genes({gene_info});

    // Set up override EC50 patterns
    GenotypeParameters::OverrideEC50Pattern override_ec50;
    override_ec50.set_pattern("||||NY1||1111111,0||||||000000000010|1");
    override_ec50.set_drug_id(1);
    override_ec50.set_ec50(0.8);

    // Set up initial parasite info
    GenotypeParameters::ParasiteInfo parasite_info;
    parasite_info.set_aa_sequence("||||YY1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
    parasite_info.set_prevalence(0.05);

    GenotypeParameters::InitialParasiteInfoRaw initial_parasite_info;
    initial_parasite_info.set_location_id(-1);
    initial_parasite_info.set_parasite_info({parasite_info});

    GenotypeParameters::PfGenotypeInfo pf_genotype_info;
    pf_genotype_info.chromosome_infos[0] = chromosome_info;

    // Set up genotype parameters
    genotype_parameters.set_mutation_mask(kMutationMask);
    genotype_parameters.set_mutation_probability_per_locus(0.001);
    genotype_parameters.set_default_cnv_reversion_multiplier(0.4);
    genotype_parameters.set_pf_genotype_info({pf_genotype_info});
    genotype_parameters.set_override_ec50_patterns({override_ec50});
    genotype_parameters.set_initial_parasite_info_raw({initial_parasite_info});
  }
};

// Test encoding functionality for GenotypeParameters
TEST_F(GenotypeParametersTest, EncodeGenotypeParameters) {
  YAML::Node node = YAML::convert<GenotypeParameters>::encode(genotype_parameters);

  // Validate encoding of genotype parameters (mutation_mask encoded as string)
  EXPECT_TRUE(node["mutation_mask"].IsScalar());
  std::string mask_str = node["mutation_mask"].as<std::string>();
  std::vector<bool> decoded_mask;
  decoded_mask.reserve(mask_str.size());
  for (char ch : mask_str) { decoded_mask.push_back(ch == '1'); }
  EXPECT_EQ(decoded_mask, kMutationMask);
  EXPECT_EQ(node["mutation_probability_per_locus"].as<double>(), 0.001);
  EXPECT_EQ(node["default_cnv_reversion_multiplier"].as<double>(), 0.4);
  EXPECT_EQ(node["pf_genotype_info"][0]["chromosome"].as<int>(), 5);
  EXPECT_EQ(node["pf_genotype_info"][0]["genes"][0]["name"].as<std::string>(), "Pfmdr1");
  EXPECT_EQ(node["pf_genotype_info"][0]["genes"][0]["cnv_reversion_multiplier"].as<double>(), 0.5);
  EXPECT_EQ(node["pf_genotype_info"][0]["genes"][0]["aa_positions"][0]["position"].as<int>(), 86);
  EXPECT_EQ(node["override_ec50_patterns"][0]["pattern"].as<std::string>(),
            "||||NY1||1111111,0||||||000000000010|1");
  EXPECT_EQ(node["initial_parasite_info"][0]["location_id"].as<int>(), -1);
}

// Test decoding functionality for GenotypeParameters
TEST_F(GenotypeParametersTest, DecodeGenotypeParametersAcceptsStringMutationMask) {
  YAML::Node node;
  node["mutation_mask"] =
      "||||111||1000000,0||||||0000000000100|1";  // 51 chars, '1'=mutable, others=immutable
  node["mutation_probability_per_locus"] = 0.001;
  node["pf_genotype_info"] = YAML::Node(YAML::NodeType::Sequence);
  node["override_ec50_patterns"] = YAML::Node(YAML::NodeType::Sequence);
  node["initial_parasite_info"] = YAML::Node(YAML::NodeType::Sequence);

  GenotypeParameters decoded_parameters;
  EXPECT_NO_THROW(YAML::convert<GenotypeParameters>::decode(node, decoded_parameters));

  // Verify the string mask was decoded correctly — only '1' = mutable
  std::vector<bool> expected_mask;
  const std::string mask_str = node["mutation_mask"].as<std::string>();
  expected_mask.reserve(mask_str.size());
  for (char ch : mask_str) { expected_mask.push_back(ch == '1'); }
  EXPECT_EQ(decoded_parameters.get_mutation_mask(), expected_mask);
}

// Test for decoding with missing fields
TEST_F(GenotypeParametersTest, DecodeGenotypeParametersMissingField) {
  // Test case: Missing mutation_probability_per_locus
  YAML::Node node1;
  node1["mutation_mask"] = make_mutation_mask_node();  // Missing mutation_probability_per_locus
  node1["pf_genotype_info"] = YAML::Node();
  node1["override_ec50_patterns"] = YAML::Node();
  node1["initial_parasite_info"] = YAML::Node();

  GenotypeParameters decoded_parameters1;
  EXPECT_THROW(YAML::convert<GenotypeParameters>::decode(node1, decoded_parameters1),
               std::runtime_error);

  // Test case: Missing pf_parasite_info
  YAML::Node node2;
  node2["mutation_mask"] = "||||111||1111111,0||||||000000000010|1";
  node2["mutation_probability_per_locus"] = 0.001;  // Missing pf_parasite_info
  node2["override_ec50_patterns"] = YAML::Node();
  node2["initial_parasite_info"] = YAML::Node();

  GenotypeParameters decoded_parameters2;
  EXPECT_THROW(YAML::convert<GenotypeParameters>::decode(node2, decoded_parameters2),
               std::runtime_error);

  // Test case: Missing override_ec50_patterns
  YAML::Node node3;
  node3["mutation_mask"] = "||||111||1111111,0||||||000000000010|1";
  node3["mutation_probability_per_locus"] = 0.001;
  node3["pf_genotype_info"] = YAML::Node();  // Missing override_ec50_patterns
  node3["initial_parasite_info"] = YAML::Node();

  GenotypeParameters decoded_parameters3;
  EXPECT_THROW(YAML::convert<GenotypeParameters>::decode(node3, decoded_parameters3),
               std::runtime_error);

  // Test case: Missing initial_parasite_info
  YAML::Node node4;
  node4["mutation_mask"] = "||||111||1111111,0||||||000000000010|1";
  node4["mutation_probability_per_locus"] = 0.001;
  node4["pf_genotype_info"] = YAML::Node();
  node4["override_ec50_patterns"] = YAML::Node();  // Missing initial_parasite_info

  GenotypeParameters decoded_parameters4;
  EXPECT_THROW(YAML::convert<GenotypeParameters>::decode(node4, decoded_parameters4),
               std::runtime_error);

  // Test case: Completely empty pf_parasite_info
  YAML::Node node5;
  node5["mutation_mask"] = "||||111||1111111,0||||||000000000010|1";
  node5["mutation_probability_per_locus"] = 0.001;
  node5["pf_genotype_info"] = YAML::Node();  // Empty pf_parasite_info
  node5["override_ec50_patterns"] = YAML::Node();
  node5["initial_parasite_info"] = YAML::Node();

  GenotypeParameters decoded_parameters5;
  EXPECT_THROW(YAML::convert<GenotypeParameters>::decode(node5, decoded_parameters5),
               std::runtime_error);

  // Test case: Missing fields inside pf_parasite_info
  YAML::Node node6;
  node6["mutation_mask"] = "||||111||1111111,0||||||000000000010|1";
  node6["mutation_probability_per_locus"] = 0.001;
  node6["pf_genotype_info"][0]["chromosome"] = 5;  // Missing genes in pf_parasite_info
  node6["override_ec50_patterns"] = YAML::Node();
  node6["initial_parasite_info"] = YAML::Node();

  GenotypeParameters decoded_parameters6;
  EXPECT_THROW(YAML::convert<GenotypeParameters>::decode(node6, decoded_parameters6),
               std::runtime_error);
}
// ----------------------------------------------------------------------------
// Out-of-bound validation tests for CNV reversion multiplier fields.
// Multipliers are accepted on [0, 10] (the upper bound is a guardrail, not a
// physical limit). These tests call GenotypeParameters's static validation
// helper directly so they don't have to set up the rest of the config.
// ----------------------------------------------------------------------------
namespace {
void install_cnv_gene(GenotypeParameters &gp, double default_multiplier, double gene_multiplier,
                      int max_copies = 2) {
  if (default_multiplier >= 0) { gp.set_default_cnv_reversion_multiplier(default_multiplier); }

  GenotypeParameters::GeneInfo gene_info;
  gene_info.set_name("Pfmdr1");
  gene_info.set_max_copies(max_copies);
  if (gene_multiplier >= 0) { gene_info.set_cnv_reversion_multiplier(gene_multiplier); }
  gene_info.set_aa_positions({});

  auto pf = gp.get_pf_genotype_info();
  pf.chromosome_infos[4].set_chromosome_id(5);
  pf.chromosome_infos[4].set_genes({gene_info});
  gp.set_pf_genotype_info(pf);
}
}  // namespace

TEST_F(GenotypeParametersTest, ValidateDefaultCnvReversionMultiplierAcceptsBoundaryValues) {
  for (double value : {0.0, 0.5, 1.0, 2.0, 5.0, 10.0}) {
    GenotypeParameters gp;
    install_cnv_gene(gp, value, -1.0);
    EXPECT_NO_THROW(GenotypeParameters::validate_cnv_reversion_multipliers(gp))
        << "default multiplier " << value << " should be accepted";
  }
}

TEST_F(GenotypeParametersTest, ValidateDefaultCnvReversionMultiplierRejectsAboveTen) {
  for (double value : {10.0001, 11.0, 100.0}) {
    GenotypeParameters gp;
    install_cnv_gene(gp, value, -1.0);
    EXPECT_THROW(GenotypeParameters::validate_cnv_reversion_multipliers(gp), std::invalid_argument)
        << "default multiplier " << value << " should be rejected";
  }
}

TEST_F(GenotypeParametersTest, ValidateDefaultCnvReversionMultiplierAcceptsSentinel) {
  // The "not configured" sentinel is -1.0. Negative values do not currently
  // throw at validation time, so the field stays inert (the reversion logic
  // treats anything < 0 as "disabled"). This test pins that contract: a
  // sentinel-valued default must not falsely flag the config as invalid.
  GenotypeParameters gp;
  install_cnv_gene(gp, -1.0, -1.0);
  EXPECT_NO_THROW(GenotypeParameters::validate_cnv_reversion_multipliers(gp));
}

TEST_F(GenotypeParametersTest, ValidateGeneCnvReversionMultiplierAcceptsBoundaryValues) {
  for (double value : {0.0, 0.5, 1.0, 2.0, 5.0, 10.0}) {
    GenotypeParameters gp;
    install_cnv_gene(gp, -1.0, value);
    EXPECT_NO_THROW(GenotypeParameters::validate_cnv_reversion_multipliers(gp))
        << "gene multiplier " << value << " should be accepted";
  }
}

TEST_F(GenotypeParametersTest, ValidateGeneCnvReversionMultiplierRejectsAboveTen) {
  for (double value : {10.0001, 11.0, 100.0}) {
    GenotypeParameters gp;
    install_cnv_gene(gp, -1.0, value);
    EXPECT_THROW(GenotypeParameters::validate_cnv_reversion_multipliers(gp), std::invalid_argument)
        << "gene multiplier " << value << " should be rejected";
  }
}

TEST_F(GenotypeParametersTest, ValidateGeneCnvReversionMultiplierRejectsOnNonCnvGene) {
  GenotypeParameters gp;
  install_cnv_gene(gp, -1.0, 0.5, /*max_copies=*/1);
  EXPECT_THROW(GenotypeParameters::validate_cnv_reversion_multipliers(gp), std::invalid_argument);
}

TEST(GenotypeParametersYamlTest, EncodesMultiMutationEc50Effect) {
  GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations effect;
  effect.set_drug_id(3);
  effect.set_factor(1.25);

  const auto node = YAML::convert<
      GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations>::encode(effect);
  EXPECT_EQ(node["drug_id"].as<int>(), 3);
  EXPECT_DOUBLE_EQ(node["factor"].as<double>(), 1.25);
}
