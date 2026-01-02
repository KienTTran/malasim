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
    // Inner class for ADC Agent
    class AdcAgent {
    public:
        // Getter and Setter
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
        bool enabled_ = false; // default disabled unless explicitly enabled in config
        std::vector<int> strategy_cycle_; // optional mapping list from manifest therapy cycle -> strategy ids
        double trigger_value_ = 0.8; // default exec threshold
        date::year_month_day trigger_date_ = date::year_month_day{}; // default / not set
    };

    // Getter and Setter for adc_agent
    [[nodiscard]] const AdcAgent &get_adc_agent() const { return adc_agent_; }
    void set_adc_agent(const AdcAgent &value) { adc_agent_ = value; }

    void process_config() override {
        spdlog::info("Using Adaptive Cycling Agent - enabled={} model_path={}",
                     adc_agent_.is_enabled(), adc_agent_.get_model_path());
    }

private:
    AdcAgent adc_agent_;
};

// YAML conversion specialization
template <>
struct YAML::convert<AgentParameters> {
    static Node encode(const AgentParameters &rhs) {
        Node node;
        node.force_insert("adc_agent", rhs.get_adc_agent());
        return node;
    }

    static bool decode(const Node &node, AgentParameters &rhs) {
        if (!node["adc_agent"]) {
            throw std::runtime_error("Missing 'adc_agent' field in agent_parameters.");
        }
        rhs.set_adc_agent(node["adc_agent"].as<AgentParameters::AdcAgent>());
        return true;
    }
};

// YAML conversion for AdcAgent
template <>
struct YAML::convert<AgentParameters::AdcAgent> {
    static Node encode(const AgentParameters::AdcAgent &rhs) {
        Node node;
        node.force_insert("model_path", rhs.get_model_path());
        node.force_insert("manifest_path", rhs.get_manifest_path());
        node.force_insert("enabled", rhs.is_enabled());
        if (!rhs.get_strategy_cycle().empty()) node.force_insert("strategy_cycle", rhs.get_strategy_cycle());
        if (rhs.get_trigger_value() != 0.8) node.force_insert("trigger_value", rhs.get_trigger_value());
        if (rhs.get_trigger_date() != date::year_month_day{}) node.force_insert("trigger_date", rhs.get_trigger_date());
        return node;
    }

    static bool decode(const Node &node, AgentParameters::AdcAgent &rhs) {
        if (!node["model_path"]) {
            throw std::runtime_error("Missing 'model_path' field in adc_agent.");
        }
        rhs.set_model_path(node["model_path"].as<std::string>());

        if (!node["manifest_path"]) {
            throw std::runtime_error("Missing 'manifest_path' field in adc_agent.");
        }
        rhs.set_manifest_path(node["manifest_path"].as<std::string>());

        // enabled is optional; default false
        if (node["enabled"]) {
            rhs.set_enabled(node["enabled"].as<bool>());
        } else {
            rhs.set_enabled(false);
        }
        if (node["strategy_cycle"]) {
            rhs.set_strategy_cycle(node["strategy_cycle"].as<std::vector<int>>());
        }
        if (node["trigger_value"]) rhs.set_trigger_value(node["trigger_value"].as<double>());
        if (node["trigger_date"]) rhs.set_trigger_date(node["trigger_date"].as<date::year_month_day>());


        return true;
    }
};

#endif //MALASIM_AGENTPARAMETERS_H