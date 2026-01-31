#ifndef EPIDEMIOLOGICALPARAMETERS_H
#define EPIDEMIOLOGICALPARAMETERS_H

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <numbers>
#include <string>
#include <spdlog/spdlog.h>
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
        double percent_mature_immunity_dont_seek_treatment = 0.0;

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
public:
    double gamma_a = 0.0;
    double gamma_b = 0.0;
    class PercentageDecidingToNotSeekTreatment {
    public:
        // Getters and Setters
        [[nodiscard]] int get_age() const { return age_; }
        void set_age(const int value) { age_ = value; }

        [[nodiscard]] double get_percentage() const { return percentage_; }
        void set_percentage(const double value) { percentage_ = value; }

    private:
        int age_ = 0;
        double percentage_ = 0.0;
    };
    [[nodiscard]] const std::vector<PercentageDecidingToNotSeekTreatment>& get_percentage_deciding_to_not_seek_treatment() const { return percentage_deciding_to_not_seek_treatment_; }
    void set_percentage_deciding_to_not_seek_treatment(const std::vector<PercentageDecidingToNotSeekTreatment>& value) { percentage_deciding_to_not_seek_treatment_ = value; }

    std::vector<PercentageDecidingToNotSeekTreatment> percentage_deciding_to_not_seek_treatment_;

    // New age-based modifier configuration
    class AgeBasedProbabilityOfSeekingTreatment {
    public:
        class PowerConfig {
        public:
            [[nodiscard]] double get_base() const { return base_; }
            void set_base(const double v) { base_ = v; }

            [[nodiscard]] const std::string& get_exponent_source() const { return exponent_source_; }
            void set_exponent_source(const std::string& s) { exponent_source_ = s; }

        private:
            double base_ = 1.0;
            std::string exponent_source_ = "index";
        };

        [[nodiscard]] bool enabled() const { return enable_; }
        void set_enable(bool v) { enable_ = v; }

        [[nodiscard]] const std::string& get_type() const { return type_; }
        void set_type(const std::string& t) { type_ = t; }

        [[nodiscard]] const PowerConfig& get_power() const { return power_; }
        void set_power(const PowerConfig& p) { power_ = p; }

        [[nodiscard]] const std::vector<int>& get_ages() const { return ages_; }
        void set_ages(const std::vector<int>& a) { ages_ = a; }

    private:
        bool enable_ = false;
        std::string type_ = "power";
        PowerConfig power_;
        std::vector<int> ages_;
    };

    [[nodiscard]] const AgeBasedProbabilityOfSeekingTreatment& get_age_based_probability_of_seeking_treatment() const { return age_based_probability_of_seeking_treatment_; }
    void set_age_based_probability_of_seeking_treatment(const AgeBasedProbabilityOfSeekingTreatment& v) { age_based_probability_of_seeking_treatment_ = v; }

    // Helper: number of buckets used for recording not-seeking-treatment counts
    [[nodiscard]] size_t get_not_seeking_treatment_bucket_count() const {
        if (age_based_probability_of_seeking_treatment_.enabled()) return age_based_probability_of_seeking_treatment_.get_ages().size();
        return percentage_deciding_to_not_seek_treatment_.size();
    }

    AgeBasedProbabilityOfSeekingTreatment age_based_probability_of_seeking_treatment_;
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

// NotProgressToClinical YAML conversion
 template<>
 struct convert<EpidemiologicalParameters::PercentageDecidingToNotSeekTreatment> {
     static Node encode(const EpidemiologicalParameters::PercentageDecidingToNotSeekTreatment& rhs) {
         Node node;
         node["age"] = rhs.get_age();
         node["percentage"] = rhs.get_percentage();
         return node;
     }

     static bool decode(const Node& node, EpidemiologicalParameters::PercentageDecidingToNotSeekTreatment& rhs) {
         if (!node["age"] || !node["percentage"]) {
             throw std::runtime_error("Missing fields in NotProgressToClinical");
         }
         rhs.set_age(node["age"].as<int>());
         rhs.set_percentage(node["percentage"].as<double>());
         return true;
     }
 };

// AgeBasedProbabilityOfSeekingTreatment YAML conversion
template<>
struct convert<EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment> {
    static Node encode(const EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment& rhs) {
        Node node;
        node["enable"] = rhs.enabled();
        node["type"] = rhs.get_type();
        Node powerNode;
        powerNode["base"] = rhs.get_power().get_base();
        powerNode["exponent_source"] = rhs.get_power().get_exponent_source();
        node["power"] = powerNode;
        node["ages"] = rhs.get_ages();
        return node;
    }

    static bool decode(const Node& node, EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment& rhs) {
        if (!node) return true;
        if (node["enable"]) rhs.set_enable(node["enable"].as<bool>());
        if (node["type"]) rhs.set_type(node["type"].as<std::string>());
        if (node["power"]) {
            EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment::PowerConfig pc;
            if (node["power"]["base"]) pc.set_base(node["power"]["base"].as<double>());
            if (node["power"]["exponent_source"]) pc.set_exponent_source(node["power"]["exponent_source"].as<std::string>());
            rhs.set_power(pc);
        }
        if (node["ages"]) rhs.set_ages(node["ages"].as<std::vector<int>>());
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
        node["percentage_deciding_to_not_seek_treatment"] = rhs.get_percentage_deciding_to_not_seek_treatment();
        node["age_based_probability_of_seeking_treatment"] = rhs.get_age_based_probability_of_seeking_treatment();
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
        if (node["percentage_deciding_to_not_seek_treatment"]) {
            rhs.set_percentage_deciding_to_not_seek_treatment(node["percentage_deciding_to_not_seek_treatment"].as<std::vector<EpidemiologicalParameters::PercentageDecidingToNotSeekTreatment>>());
        }
        if (node["age_based_probability_of_seeking_treatment"]) {
            rhs.set_age_based_probability_of_seeking_treatment(node["age_based_probability_of_seeking_treatment"].as<EpidemiologicalParameters::AgeBasedProbabilityOfSeekingTreatment>());
        }
        return true;
    }
 };
 }
 #endif //EPIDEMIOLOGICALPARAMETERS_H
