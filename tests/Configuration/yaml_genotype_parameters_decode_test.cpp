#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Configuration/GenotypeParameters.h"

namespace {
const std::vector<bool> kMutationMask{
    false, false, false, false, true,  true,  true,  false, false, true,
    false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, true,  true,  false, false, true};

YAML::Node make_decode_node() {
  YAML::Node node;
  node["mutation_mask"] = "||||111||1000000,0||||||0000000000110|1";
  node["mutation_probability_per_locus"] = 0.001;
  node["default_cnv_reversion_multiplier"] = 0.4;

  auto pf = node["pf_genotype_info"];
  pf[0]["chromosome"] = 5;
  pf[0]["genes"][0]["name"] = "Pfmdr1";
  pf[0]["genes"][0]["multiplicative_effect_on_EC50_for_2_or_more_mutations"][0]["drug_id"] = 1;
  pf[0]["genes"][0]["multiplicative_effect_on_EC50_for_2_or_more_mutations"][0]["factor"] = 1.05;
  pf[0]["genes"][0]["max_copies"] = 2;
  pf[0]["genes"][0]["cnv_reversion_multiplier"] = 0.5;
  pf[0]["genes"][0]["cnv_daily_crs"] = std::vector<double>{0.0, 0.0005};
  pf[0]["genes"][0]["cnv_multiplicative_effect_on_EC50"][0]["drug_id"] = 4;
  pf[0]["genes"][0]["cnv_multiplicative_effect_on_EC50"][0]["factors"] =
      std::vector<double>{1.0, 2.44444444};
  pf[0]["genes"][0]["cnv_multiplicative_effect_on_EC50"][1]["drug_id"] = 1;
  pf[0]["genes"][0]["cnv_multiplicative_effect_on_EC50"][1]["factors"] =
      std::vector<double>{1.0, 1.3};
  pf[0]["genes"][0]["aa_positions"][0]["position"] = 86;
  pf[0]["genes"][0]["aa_positions"][0]["amino_acids"] = std::vector<std::string>{"N", "Y"};
  pf[0]["genes"][0]["aa_positions"][0]["daily_crs"] = std::vector<double>{0.0, 0.0005};
  pf[0]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][0]["drug_id"] = 6;
  pf[0]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][0]["factors"] =
      std::vector<double>{1.0, 1.25};
  pf[0]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][1]["drug_id"] = 1;
  pf[0]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][1]["factors"] =
      std::vector<double>{1.25, 1.0};

  pf[1]["chromosome"] = 7;
  pf[1]["genes"][0]["name"] = "Pfcrt";
  pf[1]["genes"][0]["multiplicative_effect_on_EC50_for_2_or_more_mutations"][0]["drug_id"] = 3;
  pf[1]["genes"][0]["multiplicative_effect_on_EC50_for_2_or_more_mutations"][0]["factor"] = 1.0;
  pf[1]["genes"][0]["average_daily_crs"] = 0.1290;
  pf[1]["genes"][0]["aa_positions"][0]["position"] = 76;
  pf[1]["genes"][0]["aa_positions"][0]["amino_acids"] = std::vector<std::string>{"K", "T"};
  pf[1]["genes"][0]["aa_positions"][0]["daily_crs"] = std::vector<double>{0.0, 0.003875969};
  const std::vector<std::tuple<int, std::vector<double>>> pfcrt_effects{
      {6, {1.0, 1.6}}, {1, {1.1, 1.0}}, {2, {1.0, 1.2}}};
  for (std::size_t i = 0; i < pfcrt_effects.size(); ++i) {
    pf[1]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][i]["drug_id"] =
        std::get<0>(pfcrt_effects[i]);
    pf[1]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][i]["factors"] =
        std::get<1>(pfcrt_effects[i]);
  }

  pf[2]["chromosome"] = 13;
  pf[2]["genes"][0]["name"] = "Pfk13";
  pf[2]["genes"][0]["multiplicative_effect_on_EC50_for_2_or_more_mutations"][0]["drug_id"] = 0;
  pf[2]["genes"][0]["multiplicative_effect_on_EC50_for_2_or_more_mutations"][0]["factor"] = 1.1;
  pf[2]["genes"][0]["aa_positions"][0]["position"] = 446;
  pf[2]["genes"][0]["aa_positions"][0]["amino_acids"] = std::vector<std::string>{"F", "I"};
  pf[2]["genes"][0]["aa_positions"][0]["daily_crs"] = std::vector<double>{0.0, 0.0005};
  pf[2]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][0]["drug_id"] = 0;
  pf[2]["genes"][0]["aa_positions"][0]["multiplicative_effect_on_EC50"][0]["factors"] =
      std::vector<double>{1.0, 1.6648};

  pf[3]["chromosome"] = 14;
  pf[3]["genes"][0]["name"] = "Pfpm23";
  pf[3]["genes"][0]["max_copies"] = 2;
  pf[3]["genes"][0]["cnv_reversion_multiplier"] = 0.3;
  pf[3]["genes"][0]["cnv_daily_crs"] = std::vector<double>{0.0, 0.0005};
  pf[3]["genes"][0]["cnv_multiplicative_effect_on_EC50"][0]["drug_id"] = 3;
  pf[3]["genes"][0]["cnv_multiplicative_effect_on_EC50"][0]["factors"] =
      std::vector<double>{1.0, 1.37};
  pf[3]["genes"][0]["aa_positions"] = std::vector<YAML::Node>{};

  node["override_ec50_patterns"][0]["pattern"] = "||||NY1||1111111,0||||||000000000010|1";
  node["override_ec50_patterns"][0]["drug_id"] = 1;
  node["override_ec50_patterns"][0]["ec50"] = 0.8;
  node["initial_parasite_info"][0]["location_id"] = -1;
  node["initial_parasite_info"][0]["parasite_info"][0]["aa_sequence"] =
      "||||YY1||TTHFIMG,x||||||FNCMYRIPRPCRA|1";
  node["initial_parasite_info"][0]["parasite_info"][0]["prevalence"] = 0.05;
  return node;
}

template <typename Effect>
void expect_effect(const Effect &effect, int drug_id, const std::vector<double> &factors) {
  EXPECT_EQ(effect.get_drug_id(), drug_id);
  EXPECT_EQ(effect.get_factors(), factors);
}
}  // namespace

TEST(GenotypeParametersDecodeTest, DecodesChromosomeAndInitialParasiteConfiguration) {
  GenotypeParameters decoded;
  ASSERT_TRUE(YAML::convert<GenotypeParameters>::decode(make_decode_node(), decoded));

  EXPECT_EQ(decoded.get_mutation_mask(), kMutationMask);
  EXPECT_DOUBLE_EQ(decoded.get_mutation_probability_per_locus(), 0.001);
  EXPECT_DOUBLE_EQ(decoded.get_default_cnv_reversion_multiplier(), 0.4);
  EXPECT_EQ(decoded.get_override_ec50_patterns()[0].get_pattern(),
            "||||NY1||1111111,0||||||000000000010|1");
  EXPECT_EQ(decoded.get_initial_parasite_info_raw()[0].get_location_id(), -1);

  const auto &pf = decoded.get_pf_genotype_info().chromosome_infos;
  const auto &pfmdr1 = pf[4].get_genes()[0];
  EXPECT_EQ(pf[4].get_chromosome_id(), 5);
  EXPECT_EQ(pfmdr1.get_name(), "Pfmdr1");
  EXPECT_EQ(pfmdr1.get_max_copies(), 2);
  EXPECT_DOUBLE_EQ(pfmdr1.get_cnv_reversion_multiplier(), 0.5);
  EXPECT_EQ(pfmdr1.get_cnv_daily_crs(), (std::vector<double>{0.0, 0.0005}));
  expect_effect(pfmdr1.get_cnv_multiplicative_effect_on_EC50()[0], 4,
                {1.0, 2.44444444});
  expect_effect(pfmdr1.get_cnv_multiplicative_effect_on_EC50()[1], 1, {1.0, 1.3});
  const auto &mdr1_position = pfmdr1.get_aa_positions()[0];
  EXPECT_EQ(mdr1_position.get_position(), 86);
  EXPECT_EQ(mdr1_position.get_amino_acids(), (std::vector<std::string>{"N", "Y"}));
  expect_effect(mdr1_position.get_multiplicative_effect_on_EC50()[0], 6, {1.0, 1.25});
  expect_effect(mdr1_position.get_multiplicative_effect_on_EC50()[1], 1, {1.25, 1.0});

  const auto &pfcrt = pf[6].get_genes()[0];
  EXPECT_EQ(pf[6].get_chromosome_id(), 7);
  EXPECT_EQ(pfcrt.get_name(), "Pfcrt");
  EXPECT_DOUBLE_EQ(pfcrt.get_average_daily_crs(), 0.1290);
  const auto &crt_position = pfcrt.get_aa_positions()[0];
  EXPECT_EQ(crt_position.get_position(), 76);
  EXPECT_EQ(crt_position.get_amino_acids(), (std::vector<std::string>{"K", "T"}));
  EXPECT_EQ(crt_position.get_daily_crs(), (std::vector<double>{0.0, 0.003875969}));
  expect_effect(crt_position.get_multiplicative_effect_on_EC50()[0], 6, {1.0, 1.6});
  expect_effect(crt_position.get_multiplicative_effect_on_EC50()[1], 1, {1.1, 1.0});
  expect_effect(crt_position.get_multiplicative_effect_on_EC50()[2], 2, {1.0, 1.2});
}

TEST(GenotypeParametersDecodeTest, DecodesAdditionalChromosomesAndCnvDefaults) {
  GenotypeParameters decoded;
  ASSERT_TRUE(YAML::convert<GenotypeParameters>::decode(make_decode_node(), decoded));
  const auto &pf = decoded.get_pf_genotype_info().chromosome_infos;

  const auto &pfk13 = pf[12].get_genes()[0];
  EXPECT_EQ(pf[12].get_chromosome_id(), 13);
  EXPECT_EQ(pfk13.get_name(), "Pfk13");
  const auto &k13_position = pfk13.get_aa_positions()[0];
  EXPECT_EQ(k13_position.get_position(), 446);
  EXPECT_EQ(k13_position.get_amino_acids(), (std::vector<std::string>{"F", "I"}));
  EXPECT_EQ(k13_position.get_daily_crs(), (std::vector<double>{0.0, 0.0005}));
  expect_effect(k13_position.get_multiplicative_effect_on_EC50()[0], 0, {1.0, 1.6648});

  const auto &pfpm23 = pf[13].get_genes()[0];
  EXPECT_EQ(pf[13].get_chromosome_id(), 14);
  EXPECT_EQ(pfpm23.get_name(), "Pfpm23");
  EXPECT_EQ(pfpm23.get_max_copies(), 2);
  EXPECT_DOUBLE_EQ(pfpm23.get_cnv_reversion_multiplier(), 0.3);
  EXPECT_EQ(pfpm23.get_cnv_daily_crs(), (std::vector<double>{0.0, 0.0005}));
  expect_effect(pfpm23.get_cnv_multiplicative_effect_on_EC50()[0], 3, {1.0, 1.37});
  EXPECT_TRUE(pfpm23.get_aa_positions().empty());
}
