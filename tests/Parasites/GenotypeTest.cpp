#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>

#include "Parasites/Genotype.h"
#include "Population/DrugsInBlood.h"
#include "Simulation/Model.h"
#include "Treatment/Therapies/Drug.h"
#include "fixtures/TestFileGenerators.h"
#include "gtest/gtest.h"

namespace {
class FixedRandom : public utils::Random {
public:
  FixedRandom(double flat_value, double uniform_value)
      : utils::Random(nullptr, 12345), flat_value_(flat_value), uniform_value_(uniform_value) {}

  double random_flat(double from, double to) override {
    (void)from;
    (void)to;
    return flat_value_;
  }

  double random_uniform() override { return uniform_value_; }

private:
  double flat_value_;
  double uniform_value_;
};
}  // namespace

class GenotypeTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
      config["genotype_parameters"]["mutation_probability_per_locus"] = 1.0;
      config["genotype_parameters"]["default_cnv_reversion_multiplier"] = 1.0;
      config["genotype_parameters"]["pf_genotype_info"][0]["genes"][0]["cnv_reversion_multiplier"] =
          1.0;
      config["genotype_parameters"]["pf_genotype_info"][3]["genes"][0]["cnv_reversion_multiplier"] =
          1.0;
    });
    Model::get_instance()->release();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    Model::get_instance()->initialize();
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  std::string read_first_genotype_from_yaml(const std::string &yaml_path) {
    YAML::Node config = YAML::LoadFile(yaml_path);
    const auto &initial_info = config["genotype_parameters"]["initial_parasite_info"];
    if (initial_info && initial_info.IsSequence() && initial_info.size() > 0) {
      const auto &parasite_info = initial_info[0]["parasite_info"];
      if (parasite_info && parasite_info.IsSequence() && parasite_info.size() > 0) {
        return parasite_info[0]["aa_sequence"].as<std::string>();
      }
    }
    return {};
  }
  std::string read_override_pattern_from_yaml(const std::string &yaml_path) {
    YAML::Node config = YAML::LoadFile(yaml_path);
    const auto &patterns = config["genotype_parameters"]["override_ec50_patterns"];
    if (patterns && patterns.IsSequence() && patterns.size() > 0) {
      return patterns[0]["pattern"].as<std::string>();
    }
    return {};
  }

  Genotype* build_double_copy_genotype() {
    auto aa_seq = read_first_genotype_from_yaml("test_input.yml");
    auto genotype = Genotype(aa_seq);
    auto pf_genotype_str = genotype.pf_genotype_str;
    if (pf_genotype_str[4].empty() || pf_genotype_str[4][0].empty() || pf_genotype_str[13].empty()
        || pf_genotype_str[13][0].empty()) {
      return nullptr;
    }
    pf_genotype_str[4][0].back() = '2';
    pf_genotype_str[13][0].back() = '2';
    return Model::get_genotype_db()->get_genotype(
        Genotype::convert_pf_genotype_str_to_string(pf_genotype_str));
  }

  Genotype* build_multi_copy_genotype(int pfmdr1_copy_number, int pfplasmepsin_copy_number) {
    auto aa_seq = read_first_genotype_from_yaml("test_input.yml");
    auto genotype = Genotype(aa_seq);
    auto pf_genotype_str = genotype.pf_genotype_str;
    if (pf_genotype_str[4].empty() || pf_genotype_str[4][0].empty() || pf_genotype_str[13].empty()
        || pf_genotype_str[13][0].empty()) {
      return nullptr;
    }
    pf_genotype_str[4][0].back() = static_cast<char>('0' + pfmdr1_copy_number);
    pf_genotype_str[13][0].back() = static_cast<char>('0' + pfplasmepsin_copy_number);
    return Model::get_genotype_db()->get_genotype(
        Genotype::convert_pf_genotype_str_to_string(pf_genotype_str));
  }

  int get_copy_number(Genotype* genotype, int chromosome_index, int gene_index) {
    if (genotype == nullptr || genotype->pf_genotype_str[chromosome_index].size() <= gene_index
        || genotype->pf_genotype_str[chromosome_index][gene_index].empty()) {
      return -1;
    }
    return genotype->pf_genotype_str[chromosome_index][gene_index].back() - '0';
  }
};

TEST_F(GenotypeTest, ConstructorAndGetAaSequence) {
  std::string aa_seq = read_first_genotype_from_yaml("test_input.yml");
  Genotype g(aa_seq);
  EXPECT_EQ(g.get_aa_sequence(), aa_seq);
}

TEST_F(GenotypeTest, SetAndGetGenotypeId) {
  std::string aa_seq = read_first_genotype_from_yaml("test_input.yml");
  Genotype g(aa_seq);
  g.set_genotype_id(123);
  EXPECT_EQ(g.genotype_id(), 123);
}

TEST_F(GenotypeTest, CoversResistanceMutationPlaceholderAndStreamFormatting) {
  const auto aa_seq = read_first_genotype_from_yaml("test_input.yml");
  Genotype genotype(aa_seq);
  genotype.set_genotype_id(17);
  auto *drug = Model::get_drug_db()->at(0).get();
  genotype.EC50_power_n.assign(Model::get_drug_db()->size(), 1.0e6);
  EXPECT_TRUE(genotype.resist_to(drug));
  EXPECT_EQ(genotype.combine_mutation_to(2, 1), &genotype);

  std::ostringstream output;
  output << genotype;
  EXPECT_EQ(output.str(), "17\t" + aa_seq);
}

TEST_F(GenotypeTest, HandlesEmptyAlleleModificationAndInvalidGenotypeShape) {
  const auto aa_seq = read_first_genotype_from_yaml("test_input.yml");
  Genotype genotype(aa_seq);
  EXPECT_EQ(genotype.modify_genotype_allele({}, Model::get_config()),
            Model::get_genotype_db()->get_genotype(aa_seq));

  Genotype malformed("");
  EXPECT_FALSE(malformed.is_valid(Model::get_config()->get_genotype_parameters().get_pf_genotype_info()));
}

TEST_F(GenotypeTest, RejectsMalformedPfGenotypeMetadata) {
  const auto aa_seq = read_first_genotype_from_yaml("test_input.yml");
  Genotype genotype(aa_seq);
  const auto valid_info = Model::get_config()->get_genotype_parameters().get_pf_genotype_info();
  const auto chromosome_index = std::find_if(
      genotype.pf_genotype_str.begin(), genotype.pf_genotype_str.end(),
      [](const auto &genes) { return !genes.empty(); })
      - genotype.pf_genotype_str.begin();
  ASSERT_LT(chromosome_index, valid_info.chromosome_infos.size());

  auto missing_genes = valid_info;
  missing_genes.chromosome_infos[chromosome_index].set_genes({});
  EXPECT_FALSE(genotype.is_valid(missing_genes));

  auto missing_positions = valid_info;
  auto gene = missing_positions.chromosome_infos[chromosome_index].get_genes().front();
  gene.set_aa_positions({});
  missing_positions.chromosome_infos[chromosome_index].set_genes({gene});
  EXPECT_FALSE(genotype.is_valid(missing_positions));

  auto invalid_amino_acid = valid_info;
  gene = invalid_amino_acid.chromosome_infos[chromosome_index].get_genes().front();
  auto positions = gene.get_aa_positions();
  ASSERT_FALSE(positions.empty());
  positions.front().set_amino_acids({"?"});
  gene.set_aa_positions(positions);
  invalid_amino_acid.chromosome_infos[chromosome_index].set_genes({gene});
  EXPECT_FALSE(genotype.is_valid(invalid_amino_acid));

}

TEST_F(GenotypeTest, MatchPattern) {
  std::string aa_seq = read_first_genotype_from_yaml("test_input.yml");
  Genotype g(aa_seq);
  EXPECT_TRUE(g.match_pattern(aa_seq));
  std::string wildcard_pattern = aa_seq;
  wildcard_pattern[4] = '.';
  wildcard_pattern[9] = '.';
  EXPECT_TRUE(g.match_pattern(wildcard_pattern));
}

TEST_F(GenotypeTest, CnvMultiplicativeEffectChangesEC50ForDoubleCopyGene) {
  auto* genotype = build_multi_copy_genotype(2, 1);
  ASSERT_NE(genotype, nullptr);
  auto* drug = Model::get_drug_db()->at(4).get();
  const auto &pfmdr1 = Model::get_config()
                           ->get_genotype_parameters()
                           .get_pf_genotype_info()
                           .chromosome_infos[4]
                           .get_genes()[0];
  const auto cnv_factor = pfmdr1.get_cnv_multiplicative_effect_on_EC50()[0].get_factors()[1];

  const auto expected_ec50_power_n = std::pow(drug->base_ec50() * cnv_factor, drug->n());

  EXPECT_DOUBLE_EQ(genotype->get_EC50_power_n(drug), expected_ec50_power_n);
}

TEST_F(GenotypeTest, PerformCnvReversionWithoutDrugsRevertsConfiguredGenes) {
  auto* genotype = build_double_copy_genotype();
  ASSERT_NE(genotype, nullptr);
  DrugsInBlood drugs_in_blood;

  auto* reverted =
      genotype->perform_cnv_reversion(Model::get_config(), Model::get_random(), &drugs_in_blood);

  EXPECT_EQ(get_copy_number(reverted, 4, 0), 1);
  EXPECT_EQ(get_copy_number(reverted, 13, 0), 1);
}

TEST_F(GenotypeTest, PerformCnvReversionWithNonSelectingDrugStillReverts) {
  auto* genotype = build_double_copy_genotype();
  ASSERT_NE(genotype, nullptr);
  DrugsInBlood drugs_in_blood;
  auto drug = std::make_unique<Drug>(Model::get_drug_db()->at(0).get());
  drug->set_last_update_value(1.0);
  drugs_in_blood.add_drug(std::move(drug));

  auto* reverted =
      genotype->perform_cnv_reversion(Model::get_config(), Model::get_random(), &drugs_in_blood);

  EXPECT_EQ(get_copy_number(reverted, 4, 0), 1);
  EXPECT_EQ(get_copy_number(reverted, 13, 0), 1);
}

TEST_F(GenotypeTest, PerformCnvReversionSkipsGeneSelectedByDrug) {
  auto* genotype = build_double_copy_genotype();
  ASSERT_NE(genotype, nullptr);
  DrugsInBlood drugs_in_blood;
  auto selecting_drug = std::make_unique<Drug>(Model::get_drug_db()->at(1).get());
  selecting_drug->set_last_update_value(1.0);
  drugs_in_blood.add_drug(std::move(selecting_drug));

  auto* reverted =
      genotype->perform_cnv_reversion(Model::get_config(), Model::get_random(), &drugs_in_blood);

  EXPECT_EQ(get_copy_number(reverted, 4, 0), 2);
  EXPECT_EQ(get_copy_number(reverted, 13, 0), 1);
}

TEST_F(GenotypeTest, PerformCnvReversionUsesGlobalFallbackWhenGeneMultiplierMissing) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["genotype_parameters"]["mutation_probability_per_locus"] = 1.0;
    config["genotype_parameters"]["default_cnv_reversion_multiplier"] = 1.0;
    config["genotype_parameters"]["pf_genotype_info"][0]["genes"][0].remove(
        "cnv_reversion_multiplier");
    config["genotype_parameters"]["pf_genotype_info"][3]["genes"][0].remove(
        "cnv_reversion_multiplier");
  });
  Model::get_instance()->release();
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  Model::get_instance()->initialize();

  auto* genotype = build_double_copy_genotype();
  ASSERT_NE(genotype, nullptr);
  DrugsInBlood drugs_in_blood;

  auto* reverted =
      genotype->perform_cnv_reversion(Model::get_config(), Model::get_random(), &drugs_in_blood);

  EXPECT_EQ(get_copy_number(reverted, 4, 0), 1);
  EXPECT_EQ(get_copy_number(reverted, 13, 0), 1);
}

TEST_F(GenotypeTest, PerformCnvReversionReducesThreeCopiesToTwo) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["genotype_parameters"]["mutation_probability_per_locus"] = 1.0;
    config["genotype_parameters"]["default_cnv_reversion_multiplier"] = 1.0;
    config["genotype_parameters"]["pf_genotype_info"][0]["genes"][0]["max_copies"] = 3;
    config["genotype_parameters"]["pf_genotype_info"][3]["genes"][0]["max_copies"] = 3;
    config["genotype_parameters"]["pf_genotype_info"][0]["genes"][0]["cnv_reversion_multiplier"] =
        1.0;
    config["genotype_parameters"]["pf_genotype_info"][3]["genes"][0]["cnv_reversion_multiplier"] =
        1.0;
  });
  Model::get_instance()->release();
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  Model::get_instance()->initialize();

  auto* genotype = build_multi_copy_genotype(3, 3);
  ASSERT_NE(genotype, nullptr);
  DrugsInBlood drugs_in_blood;

  auto* reverted =
      genotype->perform_cnv_reversion(Model::get_config(), Model::get_random(), &drugs_in_blood);

  EXPECT_EQ(get_copy_number(reverted, 4, 0), 2);
  EXPECT_EQ(get_copy_number(reverted, 13, 0), 2);
}

TEST_F(GenotypeTest, PerformMutationByDrugCanIncreaseIntermediateCopyNumber) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["genotype_parameters"]["mutation_probability_per_locus"] = 1.0;
    config["genotype_parameters"]["pf_genotype_info"][0]["genes"][0]["max_copies"] = 3;
  });
  Model::get_instance()->release();
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  Model::get_instance()->initialize();

  auto* genotype = build_multi_copy_genotype(2, 1);
  ASSERT_NE(genotype, nullptr);
  FixedRandom mock_random(0.0, 0.75);

  auto* mutated = genotype->perform_mutation_by_drug(Model::get_config(), &mock_random,
                                                     Model::get_drug_db()->at(1).get(), 1.0);

  EXPECT_EQ(get_copy_number(mutated, 4, 0), 3);
}

TEST_F(GenotypeTest, PerformMutationByDrugCanDecreaseIntermediateCopyNumber) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["genotype_parameters"]["mutation_probability_per_locus"] = 1.0;
    config["genotype_parameters"]["pf_genotype_info"][0]["genes"][0]["max_copies"] = 3;
  });
  Model::get_instance()->release();
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  Model::get_instance()->initialize();

  auto* genotype = build_multi_copy_genotype(2, 1);
  ASSERT_NE(genotype, nullptr);
  FixedRandom mock_random(0.0, 0.25);

  auto* mutated = genotype->perform_mutation_by_drug(Model::get_config(), &mock_random,
                                                     Model::get_drug_db()->at(1).get(), 1.0);

  EXPECT_EQ(get_copy_number(mutated, 4, 0), 1);
}

TEST_F(GenotypeTest, PerformMutationByDrugAtMaximumCopyNumberStepsDown) {
  test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
    config["genotype_parameters"]["mutation_probability_per_locus"] = 1.0;
    config["genotype_parameters"]["pf_genotype_info"][0]["genes"][0]["max_copies"] = 3;
  });
  Model::get_instance()->release();
  utils::MaSimAppInput cli_input;
  cli_input.input_path = "test_input.yml";
  Model::set_cli_input(cli_input);
  Model::get_instance()->initialize();

  auto* genotype = build_multi_copy_genotype(3, 1);
  ASSERT_NE(genotype, nullptr);

  FixedRandom mock_random(0.0, 0.25);
  auto* mutated = genotype->perform_mutation_by_drug(Model::get_config(), &mock_random,
                                                     Model::get_drug_db()->at(1).get(), 1.0);

  EXPECT_EQ(get_copy_number(mutated, 4, 0), 2);
}

TEST_F(GenotypeTest, PerformMutationByDrugWithoutMutationReturnsSameGenotype) {
  auto* genotype = build_multi_copy_genotype(2, 1);
  ASSERT_NE(genotype, nullptr);
  const auto genotype_count = Model::get_genotype_db()->size();
  FixedRandom mock_random(1.0, 0.0);

  auto* mutated = genotype->perform_mutation_by_drug(Model::get_config(), &mock_random,
                                                     Model::get_drug_db()->at(1).get(), 1.0);

  EXPECT_EQ(mutated, genotype);
  EXPECT_EQ(Model::get_genotype_db()->size(), genotype_count);
}

TEST_F(GenotypeTest, CnvGeneIndicesCacheSequenceIndexAndSelectingDrugs) {
  const auto &pf_genotype_info =
      Model::get_config()->get_genotype_parameters().get_pf_genotype_info();
  const auto &cnv_gene_indices = pf_genotype_info.get_cnv_gene_indices();

  const auto pfmdr1 = std::ranges::find_if(cnv_gene_indices, [](const auto &cnv_gene_index) {
    return cnv_gene_index.chromosome_index == 4 && cnv_gene_index.gene_index == 0;
  });

  ASSERT_NE(pfmdr1, cnv_gene_indices.end());
  EXPECT_EQ(pfmdr1->aa_sequence_index,
            pf_genotype_info.calculate_aa_pos(
                4, 0, static_cast<int>(pf_genotype_info.chromosome_infos[4]
                                           .get_genes()[0]
                                           .get_aa_positions()
                                           .size())));
  EXPECT_NE(std::ranges::find(pfmdr1->selecting_drug_ids, 1),
            pfmdr1->selecting_drug_ids.end());
}
