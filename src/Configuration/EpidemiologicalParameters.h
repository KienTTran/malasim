#ifndef EPIDEMIOLOGICALPARAMETERS_H
#define EPIDEMIOLOGICALPARAMETERS_H

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <numbers>
#include <string>
#include <spdlog/spdlog.h>
#include <vector>
#include <cmath>
#include "IConfigData.h"

class EpidemiologicalParameters: public IConfigData {
public:
    class BitingLevelDistributionGamma {
    public:
        // Getters and Setters
        [[nodiscard]] double get_mean() const { return mean_; }
        void set_mean(const double value) { mean_ = value; }

        [[nodiscard]] double get_sd() const { return sd_; }
        void set_sd(const double value) { sd_ = value; }

    private:
        double mean_ = 5;
        double sd_ = 10;
    };

    class BitingLevelDistributionExponential {
    public:
        // Getters and Setters
        [[nodiscard]] double get_scale() const { return scale_; }
        void set_scale(const double value) { scale_ = value; }

    private:
        double scale_ = 0.17;
        double mean_ = 0.0;
        double sd_ = 0.0;
    };

    class BitingLevelDistribution {
    public:
        // Getters and Setters
        [[nodiscard]] const std::string& get_distribution() const { return distribution_; }
        void set_distribution(const std::string& value) { distribution_ = value; }

        [[nodiscard]] const BitingLevelDistributionGamma& get_gamma() const { return gamma_; }
        void set_gamma(const BitingLevelDistributionGamma& value) { gamma_ = value; }

        [[nodiscard]] const BitingLevelDistributionExponential& get_exponential() const { return exponential_; }
        void set_exponential(const BitingLevelDistributionExponential& value) { exponential_ = value; }

    private:
        std::string distribution_;
        BitingLevelDistributionGamma gamma_;
        BitingLevelDistributionExponential exponential_;
    };

    class RelativeBitingInfo {
    public:
        // Getters and Setters
        [[nodiscard]] int get_max_relative_biting_value() const { return max_relative_biting_value_; }
        void set_max_relative_biting_value(const int value) { max_relative_biting_value_ = value; }

        [[nodiscard]] double get_min_relative_biting_value() const { return min_relative_biting_value_; }
        void set_min_relative_biting_value(const double value) { min_relative_biting_value_ = value; }

        [[nodiscard]] int get_number_of_biting_levels() const { return number_of_biting_levels_; }
        void set_number_of_biting_levels(const int value) { number_of_biting_levels_ = value; }

        [[nodiscard]] const BitingLevelDistribution& get_biting_level_distribution() const { return biting_level_distribution_; }
        void set_biting_level_distribution(const BitingLevelDistribution& value) { biting_level_distribution_ = value; }

        [[nodiscard]] double get_scale() const { return scale_; }
        void set_scale(const double value) { scale_ = value; }

        [[nodiscard]] double get_mean() const { return mean_; }
        void set_mean(const double value) { mean_ = value; }

        [[nodiscard]] double get_sd() const { return sd_; }
        void set_sd(const double value) { sd_ = value; }

    private:
        int max_relative_biting_value_ = 35;
        double min_relative_biting_value_ = 1.0;
        int number_of_biting_levels_ = 100;
        double scale_ = 0.0;
        double mean_ = 0.0;
        double sd_ = 0.0;

        BitingLevelDistribution biting_level_distribution_;
    };

    class RelativeInfectivity {
    public:
        // Getters and Setters
        [[nodiscard]] double get_sigma() const { return sigma_; }
        void set_sigma(const double value) { sigma_ = value; }

        [[nodiscard]] double get_ro_star() const { return ro_star_; }
        void set_ro_star(const double value) { ro_star_ = value; }

        [[nodiscard]] double get_blood_meal_volume() const { return blood_meal_volume_; }
        void set_blood_meal_volume(const double value) { blood_meal_volume_ = value; }

    private:
        double sigma_ = 3.91;
        double ro_star_ = 0.00031;
        double blood_meal_volume_ = 3.0;
    };
    // New: Age-based probability of seeking treatment
    class AgeBasedProbabilityOfSeekingTreatment {
    public:
        struct PowerConfig {
            double base = 1.0; // multiplicative base
            std::string exponent_source = "index"; // how to derive exponent
        };

        [[nodiscard]] const std::string& get_type() const { return type_; }
        void set_type(const std::string& value) { type_ = value; }

        [[nodiscard]] const PowerConfig& get_power() const { return power_; }
        void set_power(const PowerConfig& value) { power_ = value; }

        [[nodiscard]] const std::vector<int>& get_ages() const { return ages_; }
        void set_ages(const std::vector<int>& value) { ages_ = value; }

        // Enabled/disabled flag
        [[nodiscard]] bool is_enabled() const { return enabled_; }
        void set_enabled(bool v) { enabled_ = v; }

        // Returns a multiplicative modifier to apply to base treatment probability
        double evaluate_for_age(const int age) const {
            if (!enabled_) return 1.0; // when disabled, no age-based modification
            if (type_.empty()) return 1.0;
            if (type_ == "power") {
                if (ages_.empty()) return 1.0;
                // exponent_source == index: find bin index based on ages vector
                if (power_.exponent_source == "index") {
                    // ages defines boundaries: ages[i] is start of bin i
                    // find largest i such that age >= ages[i]
                    int idx = 0;
                    for (size_t i = 0; i < ages_.size(); ++i) {
                        if (age >= ages_[i]) idx = static_cast<int>(i);
                        else break;
                    }
                    // exponent = idx
                    return std::pow(power_.base, idx);
                }
                // unknown exponent_source -> treat as no-op
                return 1.0;
            }
            // unknown type -> no-op
            return 1.0;
        }

        // Note: is_enabled() above returns the explicit enabled flag

     private:
         std::string type_;
         PowerConfig power_;
         std::vector<int> ages_;
         bool enabled_ = false; // default disabled when no config node is provided
     };

    // Getters and Setters
    [[nodiscard]] int get_number_of_tracking_days() const { return number_of_tracking_days_; }
    void set_number_of_tracking_days(const int value) { number_of_tracking_days_ = value; }

    [[nodiscard]] int get_days_to_clinical_under_five() const { return days_to_clinical_under_five_; }
    void set_days_to_clinical_under_five(const int value) { days_to_clinical_under_five_ = value; }

    [[nodiscard]] int get_days_to_clinical_over_five() const { return days_to_clinical_over_five_; }
    void set_days_to_clinical_over_five(const int value) { days_to_clinical_over_five_ = value; }

    [[nodiscard]] int get_days_mature_gametocyte_under_five() const { return days_mature_gametocyte_under_five_; }
    void set_days_mature_gametocyte_under_five(const int value) { days_mature_gametocyte_under_five_ = value; }

    [[nodiscard]] int get_days_mature_gametocyte_over_five() const { return days_mature_gametocyte_over_five_; }
    void set_days_mature_gametocyte_over_five(const int value) { days_mature_gametocyte_over_five_ = value; }

    [[nodiscard]] double get_p_compliance() const { return p_compliance_; }
    void set_p_compliance(const double value) { p_compliance_ = value; }

    [[nodiscard]] int get_min_dosing_days() const { return min_dosing_days_; }
    void set_min_dosing_days(const int value) { min_dosing_days_ = value; }

    [[nodiscard]] const RelativeBitingInfo& get_relative_biting_info() const { return relative_biting_info_; }
    void set_relative_biting_info(const RelativeBitingInfo& value) {
        relative_biting_info_ = value;
    }

    [[nodiscard]] double get_gametocyte_level_under_artemisinin_action() const { return gametocyte_level_under_artemisinin_action_; }
    void set_gametocyte_level_under_artemisinin_action(const double value) { gametocyte_level_under_artemisinin_action_ = value; }

    [[nodiscard]] double get_gametocyte_level_full() const { return gametocyte_level_full_; }
    void set_gametocyte_level_full(const double value) { gametocyte_level_full_ = value; }

    [[nodiscard]] const RelativeInfectivity& get_relative_infectivity() const { return relative_infectivity_; }
    void set_relative_infectivity(const RelativeInfectivity& value) { relative_infectivity_ = value; }

    [[nodiscard]] double get_p_relapse() const { return p_relapse_; }
    void set_p_relapse(const double value) { p_relapse_ = value; }

    [[nodiscard]] int get_relapse_duration() const { return relapse_duration_; }
    void set_relapse_duration(const int value) { relapse_duration_ = value; }

    [[nodiscard]] double get_relapse_rate() const { return relapse_rate_; }
    void set_relapse_rate(const double value) { relapse_rate_ = value; }

    [[nodiscard]] int get_update_frequency() const { return update_frequency_; }
    void set_update_frequency(const int value) { update_frequency_ = value; }

    [[nodiscard]] bool get_allow_new_coinfection_to_cause_symptoms() const { return allow_new_coinfection_to_cause_symptoms_; }
    void set_allow_new_coinfection_to_cause_symptoms(const bool value) { allow_new_coinfection_to_cause_symptoms_ = value; }

    [[nodiscard]] int get_tf_window_size() const { return tf_window_size_; }
    void set_tf_window_size(const int value) { tf_window_size_ = value; }

    [[nodiscard]] double get_fraction_mosquitoes_interrupted_feeding() const { return fraction_mosquitoes_interrupted_feeding_; }
    void set_fraction_mosquitoes_interrupted_feeding(const double value) { fraction_mosquitoes_interrupted_feeding_ = value; }

    [[nodiscard]] double get_inflation_factor() const { return inflation_factor_; }
    void set_inflation_factor(const double value) { inflation_factor_ = value; }

    [[nodiscard]] bool get_using_age_dependent_biting_level() const { return using_age_dependent_biting_level_; }
    void set_using_age_dependent_biting_level(const bool value) { using_age_dependent_biting_level_ = value; }

    [[nodiscard]] bool get_using_variable_probability_infectious_bites_cause_infection() const {
        return using_variable_probability_infectious_bites_cause_infection_;
    }
    void set_using_variable_probability_infectious_bites_cause_infection(const bool value) {
        using_variable_probability_infectious_bites_cause_infection_ = value;
    }

    // Accessor for new config
    [[nodiscard]] const AgeBasedProbabilityOfSeekingTreatment& get_age_based_probability_of_seeking_treatment() const { return age_based_probability_of_seeking_treatment_; }
    void set_age_based_probability_of_seeking_treatment(const AgeBasedProbabilityOfSeekingTreatment& v) { age_based_probability_of_seeking_treatment_ = v; }

    //process config data
    void process_config() override {
      spdlog::info("Processing EpidemiologicalParameters");
      const auto var = relative_biting_info_.get_biting_level_distribution().get_gamma().get_sd()
      * relative_biting_info_.get_biting_level_distribution().get_gamma().get_sd();
      gamma_b = var / relative_biting_info_.get_biting_level_distribution().get_gamma().get_mean();
      gamma_a = relative_biting_info_.get_biting_level_distribution().get_gamma().get_mean() / gamma_b;

      spdlog::info("relative_biting_info gamma_a: {}, gamma_b: {}", gamma_a, gamma_b);

      const auto d_star = 1.0/ get_relative_infectivity().get_blood_meal_volume();
      relative_infectivity_.set_ro_star((log(relative_infectivity_.get_ro_star()) - log(d_star)) / relative_infectivity_.get_sigma());
      relative_infectivity_.set_sigma(log(10) / relative_infectivity_.get_sigma());
    }

private:
    int number_of_tracking_days_ = 11;
    int days_to_clinical_under_five_ = 4;
    int days_to_clinical_over_five_ = 6;
    int days_mature_gametocyte_under_five_ = 4;
    int days_mature_gametocyte_over_five_ = 6;
    double p_compliance_ = 1.0;
    int min_dosing_days_ = 1;
    RelativeBitingInfo relative_biting_info_{};
    double gametocyte_level_under_artemisinin_action_ = 1.0;
    double gametocyte_level_full_ = 1.0;
    RelativeInfectivity relative_infectivity_{};
    double p_relapse_ = 0.01;
    int relapse_duration_ = 30;
    double relapse_rate_ = 4.4721;
    int update_frequency_ = 7;
    bool allow_new_coinfection_to_cause_symptoms_ = true;
    int tf_window_size_ = 60;
    double fraction_mosquitoes_interrupted_feeding_ = 0.0;
    double inflation_factor_ = 0.01;
    bool using_age_dependent_biting_level_ = false;
    bool using_variable_probability_infectious_bites_cause_infection_ = false;
    // new member
    AgeBasedProbabilityOfSeekingTreatment age_based_probability_of_seeking_treatment_{};
public:
    double gamma_a = 0.0;
    double gamma_b = 0.0;
};

namespace YAML {

// ExponentialDistribution YAML conversion
template<>
struct convert<EpidemiologicalParameters::BitingLevelDistributionExponential> {
    static Node encode(const EpidemiologicalParameters::BitingLevelDistributionExponential& rhs) {
        Node node;
        node["scale"] = rhs.get_scale();
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters::BitingLevelDistributionExponential& rhs) {
        if (!node["scale"]) {
            throw std::runtime_error("Missing fields in ExponentialDistribution");
        }
        rhs.set_scale(node["scale"].as<double>());
        return true;
    }
};

// GammaDistribution YAML conversion
template<>
struct convert<EpidemiologicalParameters::BitingLevelDistributionGamma> {
    static Node encode(const EpidemiologicalParameters::BitingLevelDistributionGamma& rhs) {
        Node node;
        node["mean"] = rhs.get_mean();
        node["sd"] = rhs.get_sd();
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters::BitingLevelDistributionGamma& rhs) {
        if (!node["mean"] || !node["sd"]) {
            throw std::runtime_error("Missing fields in GammaDistribution");
        }
        rhs.set_mean(node["mean"].as<double>());
        rhs.set_sd(node["sd"].as<double>());
        return true;
    }
};

// BitingLevelDistribution YAML conversion
template<>
struct convert<EpidemiologicalParameters::BitingLevelDistribution> {
    static Node encode(const EpidemiologicalParameters::BitingLevelDistribution& rhs) {
        Node node;
        node["distribution"] = rhs.get_distribution();
        node["Gamma"] = rhs.get_gamma();
        node["Exponential"] = rhs.get_exponential();
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters::BitingLevelDistribution& rhs) {
        if (!node["distribution"] || !node["Gamma"] || !node["Exponential"]) {
            throw std::runtime_error("Missing fields in BitingLevelDistribution");
        }
        rhs.set_distribution(node["distribution"].as<std::string>());
        rhs.set_gamma(node["Gamma"].as<EpidemiologicalParameters::BitingLevelDistributionGamma>());
        rhs.set_exponential(node["Exponential"].as<EpidemiologicalParameters::BitingLevelDistributionExponential>());
        return true;
    }
};

// RelativeBitingInfo YAML conversion
template<>
struct convert<EpidemiologicalParameters::RelativeBitingInfo> {
    static Node encode(const EpidemiologicalParameters::RelativeBitingInfo& rhs) {
        Node node;
        node["max_relative_biting_value"] = rhs.get_max_relative_biting_value();
        node["min_relative_biting_value"] = rhs.get_min_relative_biting_value();
        node["number_of_biting_levels"] = rhs.get_number_of_biting_levels();
        node["biting_level_distribution"] = rhs.get_biting_level_distribution();
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters::RelativeBitingInfo& rhs) {
        if (!node["max_relative_biting_value"] || !node["min_relative_biting_value"]
            || !node["number_of_biting_levels"] || !node["biting_level_distribution"]) {
            throw std::runtime_error("Missing fields in RelativeBitingInfo");
        }
        rhs.set_max_relative_biting_value(node["max_relative_biting_value"].as<int>());
        rhs.set_min_relative_biting_value(node["min_relative_biting_value"].as<double>());
        rhs.set_number_of_biting_levels(node["number_of_biting_levels"].as<int>());
        rhs.set_biting_level_distribution(node["biting_level_distribution"].as<EpidemiologicalParameters::BitingLevelDistribution>());
        return true;
    }
};

// RelativeInfectivity YAML conversion
template<>
struct convert<EpidemiologicalParameters::RelativeInfectivity> {
    static Node encode(const EpidemiologicalParameters::RelativeInfectivity& rhs) {
        Node node;
        node["sigma"] = rhs.get_sigma();
        node["ro"] = rhs.get_ro_star();
        node["blood_meal_volume"] = rhs.get_blood_meal_volume();
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters::RelativeInfectivity& rhs) {
        if (!node["sigma"] || !node["ro"] || !node["blood_meal_volume"]) {
            throw std::runtime_error("Missing fields in RelativeInfectivity");
        }
        rhs.set_sigma(node["sigma"].as<double>());
        rhs.set_ro_star(node["ro"].as<double>());
        rhs.set_blood_meal_volume(node["blood_meal_volume"].as<double>());
        return true;
    }
};

// EpidemiologicalParameters YAML conversion
template<>
struct convert<EpidemiologicalParameters> {
    static Node encode(const EpidemiologicalParameters& rhs) {
        Node node;
        node["number_of_tracking_days"] = rhs.get_number_of_tracking_days();
        node["days_to_clinical_under_five"] = rhs.get_days_to_clinical_under_five();
        node["days_to_clinical_over_five"] = rhs.get_days_to_clinical_over_five();
        node["days_mature_gametocyte_under_five"] = rhs.get_days_mature_gametocyte_under_five();
        node["days_mature_gametocyte_over_five"] = rhs.get_days_mature_gametocyte_over_five();
        node["p_compliance"] = rhs.get_p_compliance();
        node["min_dosing_days"] = rhs.get_min_dosing_days();
        node["relative_biting_info"] = rhs.get_relative_biting_info();
        node["gametocyte_level_under_artemisinin_action"] = rhs.get_gametocyte_level_under_artemisinin_action();
        node["gametocyte_level_full"] = rhs.get_gametocyte_level_full();
        node["relative_infectivity"] = rhs.get_relative_infectivity();
        node["p_relapse"] = rhs.get_p_relapse();
        node["relapse_duration"] = rhs.get_relapse_duration();
        node["relapse_rate"] = rhs.get_relapse_rate();
        node["update_frequency"] = rhs.get_update_frequency();
        node["allow_new_coinfection_to_cause_symptoms"] = rhs.get_allow_new_coinfection_to_cause_symptoms();
        node["tf_window_size"] = rhs.get_tf_window_size();
        node["fraction_mosquitoes_interrupted_feeding"] = rhs.get_fraction_mosquitoes_interrupted_feeding();
        node["inflation_factor"] = rhs.get_inflation_factor();
        node["using_age_dependent_biting_level"] = rhs.get_using_age_dependent_biting_level();
        node["using_variable_probability_infectious_bites_cause_infection"] = rhs.get_using_variable_probability_infectious_bites_cause_infection();
        // optional: age_based_probability_of_seeking_treatment
        if (rhs.get_age_based_probability_of_seeking_treatment().is_enabled()) {
            Node n;
            n["type"] = rhs.get_age_based_probability_of_seeking_treatment().get_type();
            const auto &p = rhs.get_age_based_probability_of_seeking_treatment().get_power();
            Node pnode;
            pnode["base"] = p.base;
            pnode["exponent_source"] = p.exponent_source;
            n["power"] = pnode;
            n["ages"] = rhs.get_age_based_probability_of_seeking_treatment().get_ages();
            node["age_based_probability_of_seeking_treatment"] = n;
        }
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters& rhs) {
        if (!node["number_of_tracking_days"] || !node["days_to_clinical_under_five"] || !node["days_to_clinical_over_five"]
            || !node["days_mature_gametocyte_under_five"] || !node["days_mature_gametocyte_over_five"]
            || !node["p_compliance"] || !node["min_dosing_days"] || !node["relative_biting_info"]
            || !node["gametocyte_level_under_artemisinin_action"] || !node["gametocyte_level_full"]
            || !node["relative_infectivity"] || !node["p_relapse"] || !node["relapse_duration"]
            || !node["relapse_rate"] || !node["update_frequency"] || !node["allow_new_coinfection_to_cause_symptoms"]
            || !node["tf_window_size"] || !node["fraction_mosquitoes_interrupted_feeding"]
            || !node["inflation_factor"] || !node["using_age_dependent_biting_level"]
            || !node["using_variable_probability_infectious_bites_cause_infection"]) {
            throw std::runtime_error("Missing fields in EpidemiologicalParameters");
        }
        rhs.set_number_of_tracking_days(node["number_of_tracking_days"].as<int>());
        rhs.set_days_to_clinical_under_five(node["days_to_clinical_under_five"].as<int>());
        rhs.set_days_to_clinical_over_five(node["days_to_clinical_over_five"].as<int>());
        rhs.set_days_mature_gametocyte_under_five(node["days_mature_gametocyte_under_five"].as<int>());
        rhs.set_days_mature_gametocyte_over_five(node["days_mature_gametocyte_over_five"].as<int>());
        rhs.set_p_compliance(node["p_compliance"].as<double>());
        rhs.set_min_dosing_days(node["min_dosing_days"].as<int>());
        rhs.set_relative_biting_info(node["relative_biting_info"].as<EpidemiologicalParameters::RelativeBitingInfo>());
        rhs.set_gametocyte_level_under_artemisinin_action(node["gametocyte_level_under_artemisinin_action"].as<double>());
        rhs.set_gametocyte_level_full(node["gametocyte_level_full"].as<double>());
        rhs.set_relative_infectivity(node["relative_infectivity"].as<EpidemiologicalParameters::RelativeInfectivity>());
        rhs.set_p_relapse(node["p_relapse"].as<double>());
        rhs.set_relapse_duration(node["relapse_duration"].as<int>());
        rhs.set_relapse_rate(node["relapse_rate"].as<double>());
        rhs.set_update_frequency(node["update_frequency"].as<int>());
        rhs.set_allow_new_coinfection_to_cause_symptoms(node["allow_new_coinfection_to_cause_symptoms"].as<bool>());
        rhs.set_tf_window_size(node["tf_window_size"].as<int>());
        rhs.set_fraction_mosquitoes_interrupted_feeding(node["fraction_mosquitoes_interrupted_feeding"].as<double>());
        rhs.set_inflation_factor(node["inflation_factor"].as<double>());
        rhs.set_using_age_dependent_biting_level(node["using_age_dependent_biting_level"].as<bool>());
        rhs.set_using_variable_probability_infectious_bites_cause_infection(node["using_variable_probability_infectious_bites_cause_infection"].as<bool>());
        // optional age_based_probability_of_seeking_treatment
        if (node["age_based_probability_of_seeking_treatment"]) {
            const auto n = node["age_based_probability_of_seeking_treatment"];
            EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment cfg;
            if (n["type"]) cfg.set_type(n["type"].as<std::string>());
            if (n["power"]) {
                 const auto p = n["power"];
                 EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::PowerConfig pc;
                 if (p["base"]) pc.base = p["base"].as<double>();
                 if (p["exponent_source"]) pc.exponent_source = p["exponent_source"].as<std::string>();
                 cfg.set_power(pc);
             }
            if (n["ages"]) cfg.set_ages(n["ages"].as<std::vector<int>>());
            // If the node exists but the 'enable' key is missing, default to enabled=true
            if (n["enable"]) cfg.set_enabled(n["enable"].as<bool>());
            else cfg.set_enabled(true);
             rhs.set_age_based_probability_of_seeking_treatment(cfg);
         }
        return true;
    }
};
}
#endif //EPIDEMIOLOGICALPARAMETERS_H
