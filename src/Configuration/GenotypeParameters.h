#ifndef GENOTYPEPARAMETERS_H
#define GENOTYPEPARAMETERS_H

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

#include "IConfigData.h"
#include "Parasites/GenotypeDatabase.h"

class GenotypeDatabase;

class GenotypeParameters: public IConfigData {
public:
  // Inner class: MultiplicativeEffectOnEC50For2OrMoreMutations
  class MultiplicativeEffectOnEC50For2OrMoreMutations {
  public:
    // Getters and Setters
    [[nodiscard]] int get_drug_id() const { return drug_id_; }
    void set_drug_id(const int value) { drug_id_ = value; }

    [[nodiscard]] const double& get_factor() const { return factor_; }
    void set_factor(const double& value) { factor_ = value; }

  private:
    int drug_id_ = -1;
    double factor_ = 1.0;
  };

  // Inner class: MultiplicativeEffectOnEC50
  class MultiplicativeEffectOnEC50 {
  public:
    // Getters and Setters
    [[nodiscard]] int get_drug_id() const { return drug_id_; }
    void set_drug_id(const int value) { drug_id_ = value; }

    [[nodiscard]] const std::vector<double>& get_factors() const { return factors_; }
    void set_factors(const std::vector<double>& value) { factors_ = value; }

    [[nodiscard]] std::string to_string() const {
      std::string result = std::to_string(drug_id_) + ": ";
      for (const auto& factor : factors_) {
        result += std::to_string(factor) + " ";
      }
      return result;
    }

  private:
    int drug_id_ = -1;
    std::vector<double> factors_;
  };

  // Inner class: AminoAcidPosition
  class AminoAcidPosition {
  public:
    // Getters and Setters
    [[nodiscard]] int get_position() const { return position_; }
    void set_position(const int value) { position_ = value; }

    [[nodiscard]] const std::vector<std::string>& get_amino_acids() const {
      return amino_acids_;
    }
    void set_amino_acids(const std::vector<std::string>& value) { amino_acids_ = value; }

    [[nodiscard]] const std::vector<double>& get_daily_crs() const { return daily_crs_; }
    void set_daily_crs(const std::vector<double>& value) { daily_crs_ = value; }

    [[nodiscard]] const std::vector<MultiplicativeEffectOnEC50>& get_multiplicative_effect_on_EC50() const { return multiplicative_effect_on_EC50_; }
    void set_multiplicative_effect_on_EC50(const std::vector<MultiplicativeEffectOnEC50>& value) { multiplicative_effect_on_EC50_ = value; }

    [[nodiscard]] std::string get_amino_acids_string() const {
        std::string result;
        for (const auto& aa : amino_acids_) {
            result += aa + " ";
        }
        return result;
    }

    [[nodiscard]] std::string get_daily_crs_string() const {
      std::string result;
      for (const auto& crs : daily_crs_) {
        result += std::to_string(crs) + " ";
      }
      return result;
    }

    [[nodiscard]] std::string get_multiplicative_effect_on_EC50_string() const {
      std::string result;
      for (const auto &effect : multiplicative_effect_on_EC50_) {
        result += effect.to_string() + " ";
      }
      return result;
    }

    [[nodiscard]] std::string to_string() const {
      std::string result = std::to_string(position_) + ": ";
      if (!amino_acids_.empty()) {
        for (const auto& aa : amino_acids_) {
          result += aa + " ";
        }
      }
      else {
        result = std::to_string(position_) + ": cnv";
      }
      result += "]";
      return result;
    }
  private:
    int position_ = -1;
    std::vector<std::string> amino_acids_;
    std::vector<double> daily_crs_;
    std::vector<MultiplicativeEffectOnEC50> multiplicative_effect_on_EC50_;
  };

  // Inner class: GeneInfo
  class GeneInfo {
  public:
    // Getters and Setters
    [[nodiscard]] const std::string& get_name() const { return name_; }
    void set_name(const std::string& value) { name_ = value; }

    [[nodiscard]] int get_max_copies() const { return max_copies_; }
    void set_max_copies(const int value) { max_copies_ = value; }

    [[nodiscard]] const std::vector<double>& get_cnv_daily_crs() const { return cnv_daily_crs_; }
    void set_cnv_daily_crs(const std::vector<double>& value) { cnv_daily_crs_ = value; }

    [[nodiscard]] const std::vector<MultiplicativeEffectOnEC50>& get_cnv_multiplicative_effect_on_EC50() const { return cnv_multiplicative_effect_on_EC50_; }
    void set_cnv_multiplicative_effect_on_EC50(const std::vector<MultiplicativeEffectOnEC50>& value) { cnv_multiplicative_effect_on_EC50_ = value; }

    [[nodiscard]] const std::vector<AminoAcidPosition>& get_aa_positions() const { return aa_positions_; }
    void set_aa_positions(const std::vector<AminoAcidPosition>& value) { aa_positions_ = value; }

    [[nodiscard]] const std::vector<MultiplicativeEffectOnEC50For2OrMoreMutations>& get_multiplicative_effect_on_ec50_for_2_or_more_mutations() const { return multiplicative_effect_on_ec50_for_2_or_more_mutations_; }
    void set_multiplicative_effect_on_ec50_for_2_or_more_mutations(const std::vector<MultiplicativeEffectOnEC50For2OrMoreMutations>& value) { multiplicative_effect_on_ec50_for_2_or_more_mutations_ = value; }

    [[nodiscard]] double get_average_daily_crs() const { return average_daily_crs_; }
    void set_average_daily_crs(const double value) { average_daily_crs_ = value; }

    [[nodiscard]] int get_chromosome_id() const { return chromosome_id_; }
    void set_chromosome_id(const int value) { chromosome_id_ = value; }

    [[nodiscard]] std::string to_string() const {
      std::string result = std::to_string(chromosome_id_) + ": " + name_ + " ";
      for (const auto& aa_pos : aa_positions_) {
        result += aa_pos.to_string() + " ";
      }
      return result;
    }

  private:
    int chromosome_id_ = -1;
    std::string name_;
    int max_copies_ = -1;
    std::vector<double> cnv_daily_crs_;
    std::vector<MultiplicativeEffectOnEC50> cnv_multiplicative_effect_on_EC50_;
    std::vector<AminoAcidPosition> aa_positions_;
    std::vector<MultiplicativeEffectOnEC50> multiplicative_effect_on_EC50_for_2_or_more_mutations_;
    std::vector<MultiplicativeEffectOnEC50For2OrMoreMutations> multiplicative_effect_on_ec50_for_2_or_more_mutations_;
    double average_daily_crs_ = -1;
  };

  // Inner class: ChromosomeInfo
  class ChromosomeInfo {
  public:
    // Getters and Setters
    [[nodiscard]] int get_chromosome_id() const { return chromosome_id_; }
    void set_chromosome_id(const int value) {
      chromosome_id_ = value;
    }

    [[nodiscard]] const std::vector<GeneInfo>& get_genes() const { return genes_; }
    void set_genes(const std::vector<GeneInfo>& value) {
      genes_ = value;
      for (auto & gene : genes_) {
        gene.set_chromosome_id(chromosome_id_);
      }
    }

    [[nodiscard]] std::string to_string() const {
      std::string result = std::to_string(chromosome_id_) + ": ";
      for (const auto& gene : genes_) {
        result += gene.to_string() + " ";
      }
      return result;
    }

  private:
    int chromosome_id_ = -1;
    std::vector<GeneInfo> genes_;
  };

  class PfGenotypeInfo {
  public:
    std::vector<ChromosomeInfo> chromosome_infos = std::vector<ChromosomeInfo>(14);
    [[nodiscard]] int calculate_aa_pos(const int chromosome_id,const  int gene_id, const  int aa_id) const {
      auto result = 0;
      for (int ii = 0; ii < chromosome_id; ++ii) {
        const int size = static_cast<int>(chromosome_infos[ii].get_genes().size());
        result += size > 1 ? size - 1 : 0;  // for ','
        for (const auto &gene_info : chromosome_infos[ii].get_genes()) {
          result += static_cast<int>(gene_info.get_aa_positions().size());
          result += gene_info.get_max_copies() > 1 ? 1 : 0;  // for copy number
        }
      }
      result += chromosome_id;  // for "|"

      // final chromosome
      for (int ii = 0; ii < gene_id; ++ii) {
        result += static_cast<int>(chromosome_infos[chromosome_id].get_genes()[ii].get_aa_positions().size());
        result += chromosome_infos[chromosome_id].get_genes()[ii].get_max_copies() > 1 ? 1 : 0;  // for copy number
        result += 1;                                                                      // for ","
      }

      // final gene
      result += aa_id;
      return result;
    }

    [[nodiscard]] std::string to_string() const {
      std::string result;
      for (const auto& chromosome : chromosome_infos) {
        result += chromosome.to_string() + "\n";
      }
      return result;
    }
  };

  // Inner class: OverrideEC50Pattern
  class OverrideEC50Pattern {
  public:
    // Getters and Setters
    [[nodiscard]] const std::string& get_pattern() const { return pattern_; }
    void set_pattern(const std::string& value) { pattern_ = value; }

    [[nodiscard]] int get_drug_id() const { return drug_id_; }
    void set_drug_id(const int value) { drug_id_ = value; }

    [[nodiscard]] double get_ec50() const { return ec50_; }
    void set_ec50(const double value) { ec50_ = value; }

  private:
    std::string pattern_;
    int drug_id_ = -1;
    double ec50_ = 0.0;
  };

  // Inner class: GenotypeInfo
  class ParasiteInfo {
  public:
    // Getters and Setters
    [[nodiscard]] const std::string& get_aa_sequence() const { return aa_sequence_; }
    void set_aa_sequence(const std::string& value) { aa_sequence_ = value; }

    [[nodiscard]] double get_prevalence() const { return prevalence_; }
    void set_prevalence(const double value) { prevalence_ = value; }

  private:
    std::string aa_sequence_;
    double prevalence_ = 0.0;
  };

  // Inner class: InitialParasiteInfo
  class InitialParasiteInfoRaw {
  public:
    // Getters and Setters
    [[nodiscard]] int get_location_id() const { return location_id_; }
    void set_location_id(const int value) { location_id_ = value; }

    [[nodiscard]] const std::vector<ParasiteInfo>& get_parasite_info() const { return parasite_info_; }
    void set_parasite_info(const std::vector<ParasiteInfo>& value) { parasite_info_ = value; }

  private:
    int location_id_ = -1;
    std::vector<ParasiteInfo> parasite_info_;
  };

  struct InitialParasiteInfo {
    int location;
    int parasite_type_id;
    double prevalence;

    InitialParasiteInfo() : location(-1), parasite_type_id(-1), prevalence(-1.0) {}

    InitialParasiteInfo(const int loc, const int p_type, const double pre)
        : location(loc), parasite_type_id(p_type), prevalence(pre) {}
  };

  // Getters and Setters for GenotypeParameters
  [[nodiscard]] const std::string& get_mutation_mask() const { return mutation_mask_; }
  void set_mutation_mask(const std::string& value) { mutation_mask_ = value; }

  [[nodiscard]] double get_mutation_probability_per_locus() const { return mutation_probability_per_locus_; }
  void set_mutation_probability_per_locus(const double value) { mutation_probability_per_locus_ = value; }

  [[nodiscard]] const std::vector<OverrideEC50Pattern>& get_override_ec50_patterns() const { return override_ec50_patterns_; }
  void set_override_ec50_patterns(const std::vector<OverrideEC50Pattern>& value) { override_ec50_patterns_ = value; }

  [[nodiscard]] const std::vector<InitialParasiteInfo>& get_initial_parasite_info() const { return initial_parasite_info_; }
  void set_initial_parasite_info(const std::vector<InitialParasiteInfo>& value) { initial_parasite_info_ = value; }

  [[nodiscard]] const std::vector<InitialParasiteInfoRaw>& get_initial_parasite_info_raw() const { return initial_parasite_info_raw_; }
  void set_initial_parasite_info_raw(const std::vector<InitialParasiteInfoRaw>& value) { initial_parasite_info_raw_ = value; }

  [[nodiscard]] PfGenotypeInfo get_pf_genotype_info() const { return pf_genotype_info_; }
  void set_pf_genotype_info(const PfGenotypeInfo& value) { pf_genotype_info_ = value; }

  void process_config() override {}

  void process_config_with_number_of_locations(const int number_of_locations) {
    spdlog::info("Processing GenotypeParameters");
    for (const auto &initial_genotype_info_raw : get_initial_parasite_info_raw()) {
      const auto location = initial_genotype_info_raw.get_location_id();
      const auto location_from = location == -1 ? 0 : location;
      const auto location_to =
          location == -1 ? number_of_locations : std::min<int>(location + 1, number_of_locations);

      // apply for all location
      for (auto loc = location_from; loc < location_to; ++loc) {
        for (const auto &parasite_node : initial_genotype_info_raw.get_parasite_info()) {
          const auto& aa_sequence = parasite_node.get_aa_sequence();
          auto parasite_type_id = genotype_db->get_id(aa_sequence);
          auto prevalence = parasite_node.get_prevalence();
          initial_parasite_info_.emplace_back(loc, parasite_type_id, prevalence);
        }
      }
      // for(auto &initial_genotype_info : get_initial_parasite_info()) {
        // spdlog::debug("Location: {} parasite_type_id: {} prevalence: {}",
        //              initial_genotype_info.location, initial_genotype_info.parasite_type_id,
                     // initial_genotype_info.prevalence);
      // }
    }
  }

private:
  std::string mutation_mask_;
  double mutation_probability_per_locus_ = 0.001;
  PfGenotypeInfo pf_genotype_info_;
  std::vector<OverrideEC50Pattern> override_ec50_patterns_;
  std::vector<InitialParasiteInfo> initial_parasite_info_;
  std::vector<InitialParasiteInfoRaw> initial_parasite_info_raw_;
public:
  GenotypeDatabase* genotype_db = new GenotypeDatabase();
};

namespace YAML {

// GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations YAML conversion
template<>
struct convert<GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations> {
    static Node encode(const GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations& rhs) {
        Node node;
        node["drug_id"] = rhs.get_drug_id();
        node["factor"] = rhs.get_factor();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations& rhs) {
        if (!node["drug_id"] || !node["factor"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations");
        }
        rhs.set_drug_id(node["drug_id"].as<int>());
        rhs.set_factor(node["factor"].as<double>());
        return true;
    }
};

// GenotypeParameters::MultiplicativeEffectOnEC50 YAML conversion
template<>
struct convert<GenotypeParameters::MultiplicativeEffectOnEC50> {
    static Node encode(const GenotypeParameters::MultiplicativeEffectOnEC50& rhs) {
        Node node;
        node["drug_id"] = rhs.get_drug_id();
        node["factors"] = rhs.get_factors();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::MultiplicativeEffectOnEC50& rhs) {
        if (!node["drug_id"] || !node["factors"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::MultiplicativeEffectOnEC50");
        }
        rhs.set_drug_id(node["drug_id"].as<int>());
        rhs.set_factors(node["factors"].as<std::vector<double>>());
        return true;
    }
};

// GenotypeParameters::AminoAcidPosition YAML conversion
template<>
struct convert<GenotypeParameters::AminoAcidPosition> {
    static Node encode(const GenotypeParameters::AminoAcidPosition& rhs) {
        Node node;
        node["position"] = rhs.get_position();
        node["amino_acids"] = rhs.get_amino_acids();
        node["daily_crs"] = rhs.get_daily_crs();
        node["multiplicative_effect_on_EC50"] = rhs.get_multiplicative_effect_on_EC50();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::AminoAcidPosition& rhs) {
        if (!node["position"] || !node["amino_acids"] || !node["daily_crs"] || !node["multiplicative_effect_on_EC50"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::AminoAcidPosition");
        }
        rhs.set_position(node["position"].as<int>());
        rhs.set_amino_acids(node["amino_acids"].as<std::vector<std::string>>());
        rhs.set_daily_crs(node["daily_crs"].as<std::vector<double>>());
        rhs.set_multiplicative_effect_on_EC50(node["multiplicative_effect_on_EC50"].as<std::vector<GenotypeParameters::MultiplicativeEffectOnEC50>>());
        return true;
    }
};
template<typename T>
void optional_decode(const Node& node, const std::string& key, T& value) {
    if (node[key]) {
        value = node[key].as<T>();
    }
}

// GenotypeParameters::GeneInfo YAML conversion
template<>
struct convert<GenotypeParameters::GeneInfo> {
    static Node encode(const GenotypeParameters::GeneInfo& rhs) {
        Node node;
        node["name"] = rhs.get_name();
        node["aa_positions"] = rhs.get_aa_positions();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::GeneInfo& rhs) {
        if (!node["name"] || !node["aa_positions"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::GeneInfo");
        }
        rhs.set_name(node["name"].as<std::string>());
        if(node["aa_positions"])
            rhs.set_aa_positions(node["aa_positions"].as<std::vector<GenotypeParameters::AminoAcidPosition>>());
        if(node["multiplicative_effect_on_EC50_for_2_or_more_mutations"])
            rhs.set_multiplicative_effect_on_ec50_for_2_or_more_mutations(node["multiplicative_effect_on_EC50_for_2_or_more_mutations"].as<std::vector<GenotypeParameters::MultiplicativeEffectOnEC50For2OrMoreMutations>>());
        if(node["max_copies"])
            rhs.set_max_copies(node["max_copies"].as<int>());
        if(node["cnv_daily_crs"])
            rhs.set_cnv_daily_crs(node["cnv_daily_crs"].as<std::vector<double>>());
        if(node["cnv_multiplicative_effect_on_EC50"])
            rhs.set_cnv_multiplicative_effect_on_EC50(node["cnv_multiplicative_effect_on_EC50"].as<std::vector<GenotypeParameters::MultiplicativeEffectOnEC50>>());
        if(node["average_daily_crs"])
            rhs.set_average_daily_crs(node["average_daily_crs"].as<double>());
        return true;
    }
};

// GenotypeParameters::ChromosomeInfo YAML conversion
template<>
struct convert<GenotypeParameters::ChromosomeInfo> {
    static Node encode(const GenotypeParameters::ChromosomeInfo& rhs) {
        Node node;
        node["chromosome"] = rhs.get_chromosome_id();
        node["genes"] = rhs.get_genes();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::ChromosomeInfo& rhs) {
        if (!node["chromosome"] || !node["genes"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::ChromosomeInfo");
        }
        rhs.set_chromosome_id(node["chromosome"].as<int>());
        rhs.set_genes(node["genes"].as<std::vector<GenotypeParameters::GeneInfo>>());
        return true;
    }
};

// GenotypeParameters::PfGenotypeInfo YAML conversion
template<>
struct convert<GenotypeParameters::PfGenotypeInfo> {
    static Node encode(const GenotypeParameters::PfGenotypeInfo& rhs) {
        Node node;
        node = rhs.chromosome_infos;
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::PfGenotypeInfo& rhs) {
        if (!node) {
            throw std::runtime_error("Missing fields in GenotypeParameters::PfGenotypeInfo");
        }
        for(auto const & chromosome_node : node) {
          spdlog::info("chromosome_node: {}", chromosome_node["chromosome"].as<int>());
          rhs.chromosome_infos[chromosome_node["chromosome"].as<int>() - 1] = chromosome_node.as<GenotypeParameters::ChromosomeInfo>();
        }
        return true;
    }
};

// GenotypeParameters::OverrideEC50Pattern YAML conversion
template<>
struct convert<GenotypeParameters::OverrideEC50Pattern> {
    static Node encode(const GenotypeParameters::OverrideEC50Pattern& rhs) {
        Node node;
        node["pattern"] = rhs.get_pattern();
        node["drug_id"] = rhs.get_drug_id();
        node["ec50"] = rhs.get_ec50();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::OverrideEC50Pattern& rhs) {
        if (!node["pattern"] || !node["drug_id"] || !node["ec50"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::OverrideEC50Pattern");
        }
        rhs.set_pattern(node["pattern"].as<std::string>());
        rhs.set_drug_id(node["drug_id"].as<int>());
        rhs.set_ec50(node["ec50"].as<double>());
        return true;
    }
};

// GenotypeParameters::GenotypeInfo YAML conversion
template<>
struct convert<GenotypeParameters::ParasiteInfo> {
    static Node encode(const GenotypeParameters::ParasiteInfo& rhs) {
        Node node;
        node["aa_sequence"] = rhs.get_aa_sequence();
        node["prevalence"] = rhs.get_prevalence();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::ParasiteInfo& rhs) {
        if (!node["aa_sequence"] || !node["prevalence"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::GenotypeInfo");
        }
        rhs.set_aa_sequence(node["aa_sequence"].as<std::string>());
        rhs.set_prevalence(node["prevalence"].as<double>());
        return true;
    }
};

// GenotypeParameters::InitialParasiteInfo YAML conversion
template<>
struct convert<GenotypeParameters::InitialParasiteInfoRaw> {
    static Node encode(const GenotypeParameters::InitialParasiteInfoRaw& rhs) {
        Node node;
        node["location_id"] = rhs.get_location_id();
        node["parasite_info"] = rhs.get_parasite_info();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters::InitialParasiteInfoRaw& rhs) {
        if (!node["location_id"] || !node["parasite_info"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters::InitialParasiteInfoRaw");
        }
        rhs.set_location_id(node["location_id"].as<int>());
        rhs.set_parasite_info(node["parasite_info"].as<std::vector<GenotypeParameters::ParasiteInfo>>());
        return true;
    }
};

// GenotypeParameters YAML conversion
template<>
struct convert<GenotypeParameters> {
    static Node encode(const GenotypeParameters& rhs) {
        Node node;
        node["mutation_mask"] = rhs.get_mutation_mask();
        node["mutation_probability_per_locus"] = rhs.get_mutation_probability_per_locus();
        node["pf_genotype_info"] = rhs.get_pf_genotype_info();
        node["override_ec50_patterns"] = rhs.get_override_ec50_patterns();
        node["initial_parasite_info"] = rhs.get_initial_parasite_info_raw();
        return node;
    }

    static bool decode(const Node& node, GenotypeParameters& rhs) {
        if (!node["mutation_mask"] || !node["mutation_probability_per_locus"] || !node["pf_genotype_info"]
            || !node["override_ec50_patterns"] || !node["initial_parasite_info"]) {
            throw std::runtime_error("Missing fields in GenotypeParameters");
        }
        rhs.set_mutation_mask(node["mutation_mask"].as<std::string>());
        rhs.set_mutation_probability_per_locus(node["mutation_probability_per_locus"].as<double>());
        rhs.set_pf_genotype_info(node["pf_genotype_info"].as<GenotypeParameters::PfGenotypeInfo>());
        rhs.set_override_ec50_patterns(node["override_ec50_patterns"].as<std::vector<GenotypeParameters::OverrideEC50Pattern>>());
        rhs.set_initial_parasite_info_raw(node["initial_parasite_info"].as<std::vector<GenotypeParameters::InitialParasiteInfoRaw>>());
        return true;
    }
};

}  // namespace YAML

#endif //GENOTYPEPARAMETERS_H