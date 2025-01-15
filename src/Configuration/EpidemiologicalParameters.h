#ifndef EPIDEMIOLOGICALPARAMETERS_H
#define EPIDEMIOLOGICALPARAMETERS_H

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>

#include "IConfigClass.h"

class EpidemiologicalParameters: IConfigClass {
public:
    class BitingLevelDistributionGamma {
    public:
        // Getters and Setters
        [[nodiscard]] double get_mean() const { return mean_; }
        void set_mean(double value) { mean_ = value; }

        [[nodiscard]] double get_sd() const { return sd_; }
        void set_sd(double value) { sd_ = value; }

    private:
        double mean_;
        double sd_;
    };

    class BitingLevelDistributionExponential {
    public:
        // Getters and Setters
        [[nodiscard]] double get_scale() const { return scale_; }
        void set_scale(double value) { scale_ = value; }

    private:
        double scale_;
        double mean_;
        double sd_;
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
        void set_max_relative_biting_value(int value) { max_relative_biting_value_ = value; }

        [[nodiscard]] double get_min_relative_biting_value() const { return min_relative_biting_value_; }
        void set_min_relative_biting_value(double value) { min_relative_biting_value_ = value; }

        [[nodiscard]] int get_number_of_biting_levels() const { return number_of_biting_levels_; }
        void set_number_of_biting_levels(int value) { number_of_biting_levels_ = value; }

        [[nodiscard]] const BitingLevelDistribution& get_biting_level_distribution() const { return biting_level_distribution_; }
        void set_biting_level_distribution(const BitingLevelDistribution& value) { biting_level_distribution_ = value; }

        [[nodiscard]] double get_scale() const { return scale_; }
        void set_scale(double value) { scale_ = value; }

        [[nodiscard]] double get_mean() const { return mean_; }
        void set_mean(double value) { mean_ = value; }

        [[nodiscard]] double get_sd() const { return sd_; }
        void set_sd(double value) { sd_ = value; }

        [[nodiscard]] double get_gamma_a() const { return gamma_a_; }
        void set_gamma_a(double value) { gamma_a_ = value; }

        [[nodiscard]] double get_gamma_b() const { return gamma_b_; }
        void set_gamma_b(double value) { gamma_b_ = value; }

    private:
        int max_relative_biting_value_;
        double min_relative_biting_value_;
        int number_of_biting_levels_;
        double scale_;
        double mean_;
        double sd_;
        double gamma_a_;
        double gamma_b_;

        BitingLevelDistribution biting_level_distribution_;
    };

    class RelativeInfectivity {
    public:
        // Getters and Setters
        [[nodiscard]] double get_sigma() const { return sigma_; }
        void set_sigma(double value) { sigma_ = value; }

        [[nodiscard]] double get_ro() const { return ro_; }
        void set_ro(double value) { ro_ = value; }

        [[nodiscard]] double get_blood_meal_volume() const { return blood_meal_volume_; }
        void set_blood_meal_volume(double value) { blood_meal_volume_ = value; }

    private:
        double sigma_;
        double ro_;
        double blood_meal_volume_;
    };
public:
    // Getters and Setters
    [[nodiscard]] int get_number_of_tracking_days() const { return number_of_tracking_days_; }
    void set_number_of_tracking_days(int value) { number_of_tracking_days_ = value; }

    [[nodiscard]] int get_days_to_clinical_under_five() const { return days_to_clinical_under_five_; }
    void set_days_to_clinical_under_five(int value) { days_to_clinical_under_five_ = value; }

    [[nodiscard]] int get_days_to_clinical_over_five() const { return days_to_clinical_over_five_; }
    void set_days_to_clinical_over_five(int value) { days_to_clinical_over_five_ = value; }

    [[nodiscard]] int get_days_mature_gametocyte_under_five() const { return days_mature_gametocyte_under_five_; }
    void set_days_mature_gametocyte_under_five(int value) { days_mature_gametocyte_under_five_ = value; }

    [[nodiscard]] int get_days_mature_gametocyte_over_five() const { return days_mature_gametocyte_over_five_; }
    void set_days_mature_gametocyte_over_five(int value) { days_mature_gametocyte_over_five_ = value; }

    [[nodiscard]] double get_p_compliance() const { return p_compliance_; }
    void set_p_compliance(double value) { p_compliance_ = value; }

    [[nodiscard]] int get_min_dosing_days() const { return min_dosing_days_; }
    void set_min_dosing_days(int value) { min_dosing_days_ = value; }

    [[nodiscard]] const RelativeBitingInfo& get_relative_biting_info() const { return relative_biting_info_; }
    void set_relative_biting_info(const RelativeBitingInfo& value) {
        relative_biting_info_ = value;
        // const auto temp = relative_biting_info_.get_biting_level_distribution().get_gamma().get_sd()
        // * relative_biting_info_.get_biting_level_distribution().get_gamma().get_sd();
        // relative_biting_info_.set_gamma_b(temp / relative_biting_info_.get_biting_level_distribution().get_gamma().get_mean());
        // relative_biting_info_.set_gamma_a(relative_biting_info_.get_biting_level_distribution().get_gamma().get_mean()
        //     / relative_biting_info_.get_gamma_b());
    }

    [[nodiscard]] double get_gametocyte_level_under_artemisinin_action() const { return gametocyte_level_under_artemisinin_action_; }
    void set_gametocyte_level_under_artemisinin_action(double value) { gametocyte_level_under_artemisinin_action_ = value; }

    [[nodiscard]] double get_gametocyte_level_full() const { return gametocyte_level_full_; }
    void set_gametocyte_level_full(double value) { gametocyte_level_full_ = value; }

    [[nodiscard]] const RelativeInfectivity& get_relative_infectivity() const { return relative_infectivity_; }
    void set_relative_infectivity(const RelativeInfectivity& value) { relative_infectivity_ = value; }

    [[nodiscard]] double get_p_relapse() const { return p_relapse_; }
    void set_p_relapse(double value) { p_relapse_ = value; }

    [[nodiscard]] int get_relapse_duration() const { return relapse_duration_; }
    void set_relapse_duration(int value) { relapse_duration_ = value; }

    [[nodiscard]] double get_relapse_rate() const { return relapse_rate_; }
    void set_relapse_rate(double value) { relapse_rate_ = value; }

    [[nodiscard]] int get_update_frequency() const { return update_frequency_; }
    void set_update_frequency(int value) { update_frequency_ = value; }

    [[nodiscard]] bool get_allow_new_coinfection_to_cause_symptoms() const { return allow_new_coinfection_to_cause_symptoms_; }
    void set_allow_new_coinfection_to_cause_symptoms(bool value) { allow_new_coinfection_to_cause_symptoms_ = value; }

    [[nodiscard]] int get_tf_window_size() const { return tf_window_size_; }
    void set_tf_window_size(int value) { tf_window_size_ = value; }

    [[nodiscard]] double get_fraction_mosquitoes_interrupted_feeding() const { return fraction_mosquitoes_interrupted_feeding_; }
    void set_fraction_mosquitoes_interrupted_feeding(double value) { fraction_mosquitoes_interrupted_feeding_ = value; }

    [[nodiscard]] double get_inflation_factor() const { return inflation_factor_; }
    void set_inflation_factor(double value) { inflation_factor_ = value; }

    //process config data
    void process_config() override {

    };

private:
    int number_of_tracking_days_;
    int days_to_clinical_under_five_;
    int days_to_clinical_over_five_;
    int days_mature_gametocyte_under_five_;
    int days_mature_gametocyte_over_five_;
    double p_compliance_;
    int min_dosing_days_;
    RelativeBitingInfo relative_biting_info_;
    double gametocyte_level_under_artemisinin_action_;
    double gametocyte_level_full_;
    RelativeInfectivity relative_infectivity_;
    double p_relapse_;
    int relapse_duration_;
    double relapse_rate_;
    int update_frequency_;
    bool allow_new_coinfection_to_cause_symptoms_;
    int tf_window_size_;
    double fraction_mosquitoes_interrupted_feeding_;
    double inflation_factor_;
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
        node["exponential"] = rhs.get_exponential();
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
        node["ro"] = rhs.get_ro();
        node["blood_meal_volume"] = rhs.get_blood_meal_volume();
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters::RelativeInfectivity& rhs) {
        if (!node["sigma"] || !node["ro"] || !node["blood_meal_volume"]) {
            throw std::runtime_error("Missing fields in RelativeInfectivity");
        }
        rhs.set_sigma(node["sigma"].as<double>());
        rhs.set_ro(node["ro"].as<double>());
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
        return node;
    }

    static bool decode(const YAML::Node& node, EpidemiologicalParameters& rhs) {
        if (!node["number_of_tracking_days"] || !node["days_to_clinical_under_five"] || !node["days_to_clinical_over_five"]
            || !node["days_mature_gametocyte_under_five"] || !node["days_mature_gametocyte_over_five"]
            || !node["p_compliance"] || !node["min_dosing_days"] || !node["relative_biting_info"]
            || !node["gametocyte_level_under_artemisinin_action"] || !node["gametocyte_level_full"]
            || !node["relative_infectivity"] || !node["p_relapse"] || !node["relapse_duration"]
            || !node["relapse_rate"] || !node["update_frequency"] || !node["allow_new_coinfection_to_cause_symptoms"]
            || !node["tf_window_size"] || !node["fraction_mosquitoes_interrupted_feeding"]
            || !node["inflation_factor"]) {
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
        return true;
    }
};
}
#endif //EPIDEMIOLOGICALPARAMETERS_H
