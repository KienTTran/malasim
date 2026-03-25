//
// Created by Kien Tran on 12/30/25.
//

#ifndef MALASIM_AGENTPARAMETERS_H
#define MALASIM_AGENTPARAMETERS_H

#include <yaml-cpp/yaml.h>
#include <string>
#include <date/date.h>
#include "IConfigData.h"

class AgentParameters : public IConfigData {
public:
    // ── ADC Agent (v5.5 Transformer) ─────────────────────────────────────────
    class AdcAgent {
    public:
        [[nodiscard]] const std::string &get_model_path() const { return model_path_; }
        void set_model_path(const std::string &value) { model_path_ = value; }

        [[nodiscard]] const std::string &get_manifest_path() const { return manifest_path_; }
        void set_manifest_path(const std::string &value) { manifest_path_ = value; }

        [[nodiscard]] bool is_enabled() const { return enabled_; }
        void set_enabled(bool value) { enabled_ = value; }

        [[nodiscard]] double get_trigger_value() const { return trigger_value_; }
        void set_trigger_value(double v) { trigger_value_ = v; }

        [[nodiscard]] date::year_month_day get_trigger_date() const { return trigger_date_; }
        void set_trigger_date(const date::year_month_day& d) { trigger_date_ = d; }

        [[nodiscard]] const std::vector<int>& get_strategy_cycle() const { return strategy_cycle_; }
        void set_strategy_cycle(const std::vector<int>& value) { strategy_cycle_ = value; }

    private:
        std::string model_path_;
        std::string manifest_path_;
        bool enabled_ = false;
        std::vector<int> strategy_cycle_;
        double trigger_value_ = 0.8;
        date::year_month_day trigger_date_ = date::year_month_day{};
    };

    // ── Auto Agent (IQL offline-RL actor) ────────────────────────────────────
    class AutoAgent {
    public:
        [[nodiscard]] const std::string &get_model_path() const { return model_path_; }
        void set_model_path(const std::string &value) { model_path_ = value; }

        [[nodiscard]] const std::string &get_manifest_path() const { return manifest_path_; }
        void set_manifest_path(const std::string &value) { manifest_path_ = value; }

        [[nodiscard]] bool is_enabled() const { return enabled_; }
        void set_enabled(bool value) { enabled_ = value; }

        [[nodiscard]] date::year_month_day get_trigger_date() const { return trigger_date_; }
        void set_trigger_date(const date::year_month_day& d) { trigger_date_ = d; }

        /// P(MFT) threshold: >= this → activate MFT strategy, else ADC.
        [[nodiscard]] float get_p_mft_threshold() const { return p_mft_threshold_; }
        void set_p_mft_threshold(float v) { p_mft_threshold_ = v; }

        /// Dominant therapy confidence gate: max(d_th6,d_th7,d_th8) must exceed this.
        [[nodiscard]] float get_min_dist_conf() const { return min_dist_conf_; }
        void set_min_dist_conf(float v) { min_dist_conf_ = v; }

        /// strategy_id used when agent picks ADC.
        [[nodiscard]] int get_strategy_adc() const { return strategy_adc_; }
        void set_strategy_adc(int v) { strategy_adc_ = v; }

        /// strategy_id used when agent picks MFT.
        [[nodiscard]] int get_strategy_mft() const { return strategy_mft_; }
        void set_strategy_mft(int v) { strategy_mft_ = v; }

        /// Months to block new decisions after a strategy change fires.
        [[nodiscard]] int get_cooldown_months() const { return cooldown_months_; }
        void set_cooldown_months(int v) { cooldown_months_ = v; }

    private:
        std::string model_path_;
        std::string manifest_path_;
        bool  enabled_          = false;
        float p_mft_threshold_  = 0.5f;
        float min_dist_conf_    = 0.60f;
        int   strategy_adc_     = 4;
        int   strategy_mft_     = 5;
        int   cooldown_months_  = 24;
        date::year_month_day trigger_date_ = date::year_month_day{};
    };

    // ── Getters / setters ─────────────────────────────────────────────────────
    [[nodiscard]] const AdcAgent  &get_adc_agent()  const { return adc_agent_;  }
    void set_adc_agent(const AdcAgent &value)  { adc_agent_  = value; }

    [[nodiscard]] const AutoAgent &get_auto_agent() const { return auto_agent_; }
    void set_auto_agent(const AutoAgent &value) { auto_agent_ = value; }

    void process_config() override {
        spdlog::info("ADC  agent — enabled={} model_path={}",
                     adc_agent_.is_enabled(),  adc_agent_.get_model_path());
        spdlog::info("Auto agent — enabled={} model_path={}",
                     auto_agent_.is_enabled(), auto_agent_.get_model_path());
    }

private:
    AdcAgent  adc_agent_;
    AutoAgent auto_agent_;
};

// ── YAML: AgentParameters ─────────────────────────────────────────────────────
template <>
struct YAML::convert<AgentParameters> {
    static Node encode(const AgentParameters &rhs) {
        Node node;
        node.force_insert("adc_agent",  rhs.get_adc_agent());
        node.force_insert("auto_agent", rhs.get_auto_agent());
        return node;
    }

    static bool decode(const Node &node, AgentParameters &rhs) {
        // adc_agent is optional — existing configs without it still load fine
        if (node["adc_agent"])
            rhs.set_adc_agent(node["adc_agent"].as<AgentParameters::AdcAgent>());

        // auto_agent is optional — disabled by default if absent
        if (node["auto_agent"])
            rhs.set_auto_agent(node["auto_agent"].as<AgentParameters::AutoAgent>());

        return true;
    }
};

// ── YAML: AdcAgent ────────────────────────────────────────────────────────────
template <>
struct YAML::convert<AgentParameters::AdcAgent> {
    static Node encode(const AgentParameters::AdcAgent &rhs) {
        Node node;
        node.force_insert("model_path",    rhs.get_model_path());
        node.force_insert("manifest_path", rhs.get_manifest_path());
        node.force_insert("enabled",       rhs.is_enabled());
        if (!rhs.get_strategy_cycle().empty())
            node.force_insert("strategy_cycle", rhs.get_strategy_cycle());
        if (rhs.get_trigger_value() != 0.8)
            node.force_insert("trigger_value", rhs.get_trigger_value());
        if (rhs.get_trigger_date() != date::year_month_day{})
            node.force_insert("trigger_date", rhs.get_trigger_date());
        return node;
    }

    static bool decode(const Node &node, AgentParameters::AdcAgent &rhs) {
        if (!node["model_path"])
            throw std::runtime_error("Missing 'model_path' in adc_agent.");
        rhs.set_model_path(node["model_path"].as<std::string>());

        if (!node["manifest_path"])
            throw std::runtime_error("Missing 'manifest_path' in adc_agent.");
        rhs.set_manifest_path(node["manifest_path"].as<std::string>());

        rhs.set_enabled(node["enabled"] ? node["enabled"].as<bool>() : false);

        if (node["strategy_cycle"])
            rhs.set_strategy_cycle(node["strategy_cycle"].as<std::vector<int>>());
        if (node["trigger_value"])
            rhs.set_trigger_value(node["trigger_value"].as<double>());
        if (node["trigger_date"])
            rhs.set_trigger_date(node["trigger_date"].as<date::year_month_day>());

        return true;
    }
};

// ── YAML: AutoAgent ───────────────────────────────────────────────────────────
template <>
struct YAML::convert<AgentParameters::AutoAgent> {
    static Node encode(const AgentParameters::AutoAgent &rhs) {
        Node node;
        node.force_insert("model_path",      rhs.get_model_path());
        node.force_insert("manifest_path",   rhs.get_manifest_path());
        node.force_insert("enabled",         rhs.is_enabled());
        node.force_insert("p_mft_threshold", rhs.get_p_mft_threshold());
        node.force_insert("min_dist_conf",   rhs.get_min_dist_conf());
        node.force_insert("strategy_adc",    rhs.get_strategy_adc());
        node.force_insert("strategy_mft",    rhs.get_strategy_mft());
        node.force_insert("cooldown_months", rhs.get_cooldown_months());
        if (rhs.get_trigger_date() != date::year_month_day{})
            node.force_insert("trigger_date", rhs.get_trigger_date());
        return node;
    }

    static bool decode(const Node &node, AgentParameters::AutoAgent &rhs) {
        if (!node["model_path"])
            throw std::runtime_error("Missing 'model_path' in auto_agent.");
        rhs.set_model_path(node["model_path"].as<std::string>());

        if (!node["manifest_path"])
            throw std::runtime_error("Missing 'manifest_path' in auto_agent.");
        rhs.set_manifest_path(node["manifest_path"].as<std::string>());

        rhs.set_enabled(node["enabled"] ? node["enabled"].as<bool>() : false);

        if (node["p_mft_threshold"])
            rhs.set_p_mft_threshold(node["p_mft_threshold"].as<float>());
        if (node["min_dist_conf"])
            rhs.set_min_dist_conf(node["min_dist_conf"].as<float>());
        if (node["strategy_adc"])
            rhs.set_strategy_adc(node["strategy_adc"].as<int>());
        if (node["strategy_mft"])
            rhs.set_strategy_mft(node["strategy_mft"].as<int>());
        if (node["cooldown_months"])
            rhs.set_cooldown_months(node["cooldown_months"].as<int>());
        if (node["trigger_date"])
            rhs.set_trigger_date(node["trigger_date"].as<date::year_month_day>());

        return true;
    }
};

#endif //MALASIM_AGENTPARAMETERS_H
