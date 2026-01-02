#ifndef ADAPTIVECYCLINGAGENT_H
#define ADAPTIVECYCLINGAGENT_H

#include <torch/script.h>
#include <yaml-cpp/yaml.h>

#include <array>
#include <cmath>
#include <deque>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "ModelLoader.h"
#include "Reporters/Reporter.h"
#include "Reporters/SQLiteValidationReporter.h"

class ModelDataCollector;

class AdaptiveCyclingAgent {
private:
    struct ADCTorchMeta {
        int L = 0;   // history window length (months)
        int H = 0;   // horizon
        int F = 0;   // number of input features
        int K = 0;   // number of classes
        std::vector<std::string> input_features;          // exact training order
        std::unordered_map<int,int> class_to_therapy;     // class -> therapy_id
    };

    struct ADCTorchModel {
        ADCTorchMeta meta;
        torch::jit::Module module;
        torch::Device device = torch::kCPU;
    };

    torch::jit::Module agent_raw_model_;
    YAML::Node agent_raw_manifest_;
    ADCTorchMeta agent_meta_;
    ADCTorchModel agent_model_;

public:
    struct ADCState {
        bool deployed = false;

        // Config (copied here for convenience)
        double trigger_threshold = 0.0;
        int delay_until_actual_trigger_days = 0;
        int turn_off_days_default = 0;
        std::array<int, 3> therapy_ids = {8, 7, 6};

        // Runtime counters (persistent)
        int delay_days_left = 0;
        int turn_off_days_left = 0;

        // Optional
        double trigger_value_at_start = 0.0;
        int active_therapy_index = 0;
    };

    struct ADCInferenceDebug {
        int level_id = -1;
        int unit_id = -1;
        int end_month = -1;          // exclusive
        int predicted_class = -1;
        int predicted_therapy = -1;

        std::vector<float> logits;              // [K]
        std::vector<float> probabilities;       // [K] optional
        std::vector<float> input_last_timestep; // [F]
    };

    struct ADCAgentData {
        // --------- monthly feature vectors (reset each month) ----------
        std::vector<double> current_tf_by_unit;
        std::vector<double> monthly_number_of_tf_by_unit;
        std::vector<double> accumulative_tf_by_unit;
        std::vector<double> accumulative_ntf_by_unit;

        std::vector<double> tf_by_therapy_6_by_unit;
        std::vector<double> tf_by_therapy_7_by_unit;
        std::vector<double> tf_by_therapy_8_by_unit;

        std::vector<double> current_580Y_freq_by_unit;

        std::vector<double> adc_deployed_by_unit;
        std::vector<double> adc_trigger_value_by_unit;
        std::vector<double> adc_delay_days_by_unit;
        std::vector<double> adc_turn_off_days_by_unit;

        // --------- month history (deque for cheap pop_front) ----------
        std::deque<std::vector<double>> current_tf_hist;
        std::deque<std::vector<double>> monthly_tf_hist;
        std::deque<std::vector<double>> acc_tf_hist;
        std::deque<std::vector<double>> acc_ntf_hist;

        std::deque<std::vector<double>> tf6_hist;
        std::deque<std::vector<double>> tf7_hist;
        std::deque<std::vector<double>> tf8_hist;

        std::deque<std::vector<double>> freq_580Y_hist;

        std::deque<std::vector<double>> adc_deployed_hist;
        std::deque<std::vector<double>> adc_trigger_value_hist;
        std::deque<std::vector<double>> adc_delay_days_hist;
        std::deque<std::vector<double>> adc_turn_off_days_hist;

        int max_history_months = 0;

        // --- ADC scheduling guard (per unit) ---
        std::vector<int> pending_switch_day_by_unit;      // -1 = none, else absolute day
        std::vector<int> pending_switch_therapy_by_unit;  // -1 = none, else therapy id
        std::vector<int> pending_switch_strategy_by_unit; // -1 = none, else strategy id

        // block inference until this day (inclusive/exclusive is your choice; I use < day to block)
        std::vector<int> block_inference_until_day_by_unit; // -1 = no block


        // --------- persistent controller state (DO NOT reset) ----------
        std::vector<ADCState> adc_state_by_unit;

        // ---- sizing / config ----
        void ensure_adc_state_size_and_init(
            int vector_size,
            double trigger_threshold,
            int delay_until_actual_trigger_days,
            int turn_off_days_default,
            std::array<int,3> therapy_ids
        ) {
            if ((int)adc_state_by_unit.size() == vector_size) {
                for (auto& s : adc_state_by_unit) {
                    s.trigger_threshold = trigger_threshold;
                    s.delay_until_actual_trigger_days = delay_until_actual_trigger_days;
                    s.turn_off_days_default = turn_off_days_default;
                    s.therapy_ids = therapy_ids;
                }
                return;
            }

            adc_state_by_unit.resize(vector_size);
            for (auto& s : adc_state_by_unit) {
                s.deployed = false;
                s.trigger_threshold = trigger_threshold;
                s.delay_until_actual_trigger_days = delay_until_actual_trigger_days;
                s.turn_off_days_default = turn_off_days_default;
                s.therapy_ids = therapy_ids;

                s.delay_days_left = 0;
                s.turn_off_days_left = 0;
                s.trigger_value_at_start = 0.0;
                s.active_therapy_index = 0;
            }
        }

        // Keep enough months for model window + rolling features (+1 buffer for delta)
        void set_history_cap(int L_months, int rolling_window, int user_cap_months = 0) {
            int auto_cap = L_months + rolling_window + 1;
            max_history_months = (user_cap_months > 0) ? user_cap_months : auto_cap;
            if (max_history_months < auto_cap) max_history_months = auto_cap;
        }

        // reset only the monthly vectors (NOT persistent adc_state_by_unit)
        void reset_month(int vector_size) {
            current_tf_by_unit.assign(vector_size, 0.0);
            monthly_number_of_tf_by_unit.assign(vector_size, 0.0);
            accumulative_tf_by_unit.assign(vector_size, 0.0);
            accumulative_ntf_by_unit.assign(vector_size, 0.0);

            tf_by_therapy_6_by_unit.assign(vector_size, 0.0);
            tf_by_therapy_7_by_unit.assign(vector_size, 0.0);
            tf_by_therapy_8_by_unit.assign(vector_size, 0.0);

            current_580Y_freq_by_unit.assign(vector_size, 0.0);

            adc_deployed_by_unit.assign(vector_size, 0.0);
            adc_trigger_value_by_unit.assign(vector_size, 0.0);
            adc_delay_days_by_unit.assign(vector_size, 0.0);
            adc_turn_off_days_by_unit.assign(vector_size, 0.0);
        }

        void ensure_guard_size(int vector_size) {
            if ((int)pending_switch_day_by_unit.size() != vector_size) {
                pending_switch_day_by_unit.assign(vector_size, -1);
                pending_switch_therapy_by_unit.assign(vector_size, -1);
                pending_switch_strategy_by_unit.assign(vector_size, -1);
                block_inference_until_day_by_unit.assign(vector_size, -1);
            }
        }


        void snapshot_adc_state_to_features() {
            const int n = (int)adc_state_by_unit.size();
            if ((int)adc_deployed_by_unit.size() != n) return;

            for (int u = 0; u < n; ++u) {
                const auto& s = adc_state_by_unit[u];
                adc_deployed_by_unit[u] = s.deployed ? 1.0 : 0.0;

                // choose meaning you trained with; here: threshold/config
                adc_trigger_value_by_unit[u] = s.trigger_threshold;

                adc_delay_days_by_unit[u] = (double)s.delay_days_left;
                adc_turn_off_days_by_unit[u] = (double)s.turn_off_days_left;
            }
        }

        int history_len() const {
            return (int)freq_580Y_hist.size();
        }

        static void trim_deque(std::deque<std::vector<double>>& q, int cap) {
            while (cap > 0 && (int)q.size() > cap) q.pop_front();
        }

        void push_month_all() {
            // push snapshots
            current_tf_hist.push_back(current_tf_by_unit);
            monthly_tf_hist.push_back(monthly_number_of_tf_by_unit);
            acc_tf_hist.push_back(accumulative_tf_by_unit);
            acc_ntf_hist.push_back(accumulative_ntf_by_unit);

            tf6_hist.push_back(tf_by_therapy_6_by_unit);
            tf7_hist.push_back(tf_by_therapy_7_by_unit);
            tf8_hist.push_back(tf_by_therapy_8_by_unit);

            freq_580Y_hist.push_back(current_580Y_freq_by_unit);

            adc_deployed_hist.push_back(adc_deployed_by_unit);
            adc_trigger_value_hist.push_back(adc_trigger_value_by_unit);
            adc_delay_days_hist.push_back(adc_delay_days_by_unit);
            adc_turn_off_days_hist.push_back(adc_turn_off_days_by_unit);

            // trim to cap
            const int cap = max_history_months;
            trim_deque(current_tf_hist, cap);
            trim_deque(monthly_tf_hist, cap);
            trim_deque(acc_tf_hist, cap);
            trim_deque(acc_ntf_hist, cap);

            trim_deque(tf6_hist, cap);
            trim_deque(tf7_hist, cap);
            trim_deque(tf8_hist, cap);

            trim_deque(freq_580Y_hist, cap);

            trim_deque(adc_deployed_hist, cap);
            trim_deque(adc_trigger_value_hist, cap);
            trim_deque(adc_delay_days_hist, cap);
            trim_deque(adc_turn_off_days_hist, cap);
        }

        // ---- derived genotype helpers (in history index space) ----
        double freq_580Y_delta(int t, int u) const {
            if (t <= 0) return 0.0;
            return freq_580Y_hist[t][u] - freq_580Y_hist[t - 1][u];
        }

        double freq_580Y_rollmean_12(int t, int u) const {
            const int window = 12;
            const int start = std::max(0, t - window + 1);
            double sum = 0.0;
            int count = 0;
            for (int m = start; m <= t; ++m) {
                sum += freq_580Y_hist[m][u];
                ++count;
            }
            return count ? sum / count : 0.0;
        }

        // Build model batch: [num_units x (L*F)] in exact feature order
        std::vector<std::vector<float>> build_input_batch(
            int end_month_exclusive,
            int L,
            const std::vector<std::string>& feature_names
        ) const;
    };

    std::vector<ADCAgentData> adc_agent_data_by_level;

    struct YearMonth {
        int year;
        int month; // 1..12
    };

    // Mapping from therapy id -> strategy id, loaded from configuration's adc_agent.strategy_cycle
    std::unordered_map<int, std::vector<int>> therapy_strategy_map_;
    std::unordered_map<int, int> therapy_cycle_index_;

    // ADC agent-level trigger parameters (loaded from AgentParameters.adc_agent)
    double trigger_value_ = 0.8; // default exec threshold
    long trigger_day_ = 1;
    int trigger_month_ = -1; // days from simulation starting_date to adc trigger_date

    // Simulation start date components (set from configuration)
    int sim_start_year_ = 2000;
    int sim_start_month_ = 0; // user requested month 0

    // ------------------------------
    // Helpers
    // ------------------------------
    static int as_int_or_throw(const YAML::Node& n, const char* path) {
        if (!n) {
            throw std::runtime_error(std::string("Missing YAML field: ") + path);
        }
        if (!n.IsScalar()) {
            throw std::runtime_error(std::string("YAML field not scalar: ") + path);
        }
        try {
            return n.as<int>();
        } catch (const YAML::BadConversion& e) {
            throw std::runtime_error(std::string("Bad int conversion at ") + path +
                                     " value='" + n.Scalar() + "' : " + e.what());
        }
    }

    static std::string as_str_or_throw(const YAML::Node& n, const char* path) {
        if (!n) throw std::runtime_error(std::string("Missing YAML field: ") + path);
        if (!n.IsScalar()) throw std::runtime_error(std::string("YAML field not scalar: ") + path);
        return n.as<std::string>();
    }

    // ------------------------------
    // TorchScript forward helpers
    // ------------------------------
    struct ADCOutputs {
        torch::Tensor exec_logits; // (N,H)
        torch::Tensor excc_logits; // (N,H,K)
    };

    static inline float sigmoidf(float x) {
        if (x >= 0.f) {
            float z = std::exp(-x);
            return 1.f / (1.f + z);
        } else {
            float z = std::exp(x);
            return z / (1.f + z);
        }
    }

    static inline void softmax_row(const std::vector<float>& logits, std::vector<float>& probs) {
        probs.resize(logits.size());
        if (logits.empty()) return;

        float mx = *std::max_element(logits.begin(), logits.end());
        float sum = 0.f;
        for (size_t i = 0; i < logits.size(); ++i) {
            probs[i] = std::exp(logits[i] - mx);
            sum += probs[i];
        }
        sum = std::max(sum, 1e-12f);
        for (size_t i = 0; i < probs.size(); ++i) probs[i] /= sum;
    }

    static ADCOutputs forward_adc_outputs(torch::jit::Module& module,
                                          const std::vector<torch::jit::IValue>& inputs) {
        torch::jit::IValue out = module.forward(inputs);

        if (!out.isTuple()) {
            throw std::runtime_error("ADC model forward() must return tuple (exec_logits, excc_logits).");
        }

        auto tup = out.toTuple();
        const auto& elems = tup->elements();
        if (elems.size() != 2) {
            throw std::runtime_error("ADC model forward tuple must have exactly 2 elements.");
        }
        if (!elems[0].isTensor() || !elems[1].isTensor()) {
            throw std::runtime_error("ADC model forward tuple elements must be tensors.");
        }

        ADCOutputs o;
        o.exec_logits = elems[0].toTensor();
        o.excc_logits = elems[1].toTensor();
        return o;
    }

    struct ADCTop1Decision {
        int best_h = -1;
        int therapy = -1;
        int forecast_day = -1;
        float exec_p = 0.f;
        float cls_p = 0.f;
    };


public:
    AdaptiveCyclingAgent(const AdaptiveCyclingAgent&) = delete;
    AdaptiveCyclingAgent& operator=(const AdaptiveCyclingAgent&) = delete;
    AdaptiveCyclingAgent(AdaptiveCyclingAgent&&) = delete;
    AdaptiveCyclingAgent& operator=(AdaptiveCyclingAgent&&) = delete;

    explicit AdaptiveCyclingAgent();
    virtual ~AdaptiveCyclingAgent() = default;

    void load_model();
    void load_manifest();
    void initialize();

    YAML::Node get_manifest() const { return agent_raw_manifest_; }

    void monthly_collect_data_by_level(int level_id);
    void reset_adc_data(int level_id, int vector_size);

    void finalize_month_580Y_freq(
        int level_id,
        int numGenotypes,
        std::vector<SQLiteValidationReporter::MonthlyGenomeData> monthly_genome_data_by_level
    );

    // Inference using internal ADCAgentData history
    void inference_from_adc_data(int level_id, std::vector<int>& output_classes);

    // Debug inference (captures logits/probs/last-timestep)
    void inference_from_adc_data_debug(
        int level_id,
        std::vector<ADCInferenceDebug>& debug_out,
        bool compute_softmax
    );

    static void log_adc_inference_debug(
        const ADCInferenceDebug& d,
        const std::vector<std::string>& feature_names
    );

    static YearMonth add_months(int start_year, int start_month, int offset);

    static std::string ym_to_string(const YearMonth& ym);

    void print_adc_schedule(
        int tcur_month,
        int tcur_day,                      // NEW: current simulation day
        int horizon,
        const std::vector<float>& p_exec,
        const std::vector<std::vector<float>>& p_cls,
        const std::vector<int>& class_to_therapy,
        float exec_threshold,
        int sim_start_year,
        int sim_start_month,
        int days_per_month                 // NEW: usually 30
    ) ;

    void print_adc_schedule_top1(
        int tcur_month,
        int tcur_day,
        int horizon,
        const std::vector<float>& p_exec,
        const std::vector<std::vector<float>>& p_cls,
        const std::vector<int>& class_to_therapy,
        float exec_threshold,
        int sim_start_year,
        int sim_start_month,
        int days_per_month
    );

};

#endif // ADAPTIVECYCLINGAGENT_H
