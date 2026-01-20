#include "gtest/gtest.h"
#include "Parasites/Genotype.h"
#include <string>
#include <fstream>
#include <yaml-cpp/yaml.h>

class GenotypeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    std::string read_first_genotype_from_yaml(const std::string& yaml_path) {
        YAML::Node config = YAML::LoadFile(yaml_path);
        const auto& initial_info = config["initial_parasite_info"];
        if (initial_info && initial_info.IsSequence() && initial_info.size() > 0) {
            const auto& parasite_info = initial_info[0]["parasite_info"];
            if (parasite_info && parasite_info.IsSequence() && parasite_info.size() > 0) {
                return parasite_info[0]["aa_sequence"].as<std::string>();
            }
        }
        return {};
    }
    std::string read_override_pattern_from_yaml(const std::string& yaml_path) {
        YAML::Node config = YAML::LoadFile(yaml_path);
        const auto& patterns = config["genotype_parameters"]["override_ec50_patterns"];
        if (patterns && patterns.IsSequence() && patterns.size() > 0) {
            return patterns[0]["pattern"].as<std::string>();
        }
        return {};
    }
};

TEST_F(GenotypeTest, ConstructorAndGetAaSequence) {
    std::string aa_seq = read_first_genotype_from_yaml("sample_inputs/input.yml");
    Genotype g(aa_seq);
    EXPECT_EQ(g.get_aa_sequence(), aa_seq);
}

TEST_F(GenotypeTest, SetAndGetGenotypeId) {
    std::string aa_seq = read_first_genotype_from_yaml("sample_inputs/input.yml");
    Genotype g(aa_seq);
    g.set_genotype_id(123);
    EXPECT_EQ(g.genotype_id(), 123);
}

TEST_F(GenotypeTest, MatchPattern) {
    std::string aa_seq = read_first_genotype_from_yaml("sample_inputs/input.yml");
    Genotype g(aa_seq);
    EXPECT_TRUE(g.match_pattern(aa_seq));
    std::string override_pattern = read_override_pattern_from_yaml("sample_inputs/input.yml");
    EXPECT_TRUE(g.match_pattern(override_pattern));
}

// Additional tests for mutation, recombination, etc. can be added here
