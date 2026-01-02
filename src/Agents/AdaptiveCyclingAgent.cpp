#include "AdaptiveCyclingAgent.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <date/date.h>
#include <chrono>

#include "Events/Population/ChangeTreatmentStrategyEvent.h"
#include "Reporters/SQLiteMonthlyReporter.h"
#include "Reporters/SQLiteValidationReporter.h"
#include "Simulation/Model.h"

// ------------------------------
// AdaptiveCyclingAgent
// ------------------------------
AdaptiveCyclingAgent::AdaptiveCyclingAgent() = default;

void AdaptiveCyclingAgent::load_model() {
    agent_raw_model_ = ModelLoader::load_model(
        Model::get_config()->get_agent_parameters().get_adc_agent().get_model_path()
    );

    // eval mode for inference
    agent_raw_model_.eval();

    agent_model_ = ADCTorchModel{
        .meta   = agent_meta_,
        .module = agent_raw_model_,
        .device = torch::kCPU
    };

    spdlog::info("ADC model loaded: {}",
                 Model::get_config()->get_agent_parameters().get_adc_agent().get_model_path());
}

void AdaptiveCyclingAgent::load_manifest() {
    const auto manifest_path =
        Model::get_config()->get_agent_parameters().get_adc_agent().get_manifest_path();

    spdlog::info("Loading ADC manifest from: {}", manifest_path);

    agent_raw_manifest_ = ModelLoader::load_manifest(manifest_path);

    // --- window ---
    agent_meta_.L = as_int_or_throw(agent_raw_manifest_["window"]["L"], "window.L");
    agent_meta_.H = as_int_or_throw(agent_raw_manifest_["window"]["H"], "window.H");

    // --- input features ---
    const YAML::Node feats = agent_raw_manifest_["input_features"];
    if (!feats || !feats.IsSequence()) {
        throw std::runtime_error("input_features missing or not a sequence");
    }
    agent_meta_.input_features.clear();
    agent_meta_.input_features.reserve(feats.size());
    for (size_t i = 0; i < feats.size(); ++i) {
        agent_meta_.input_features.push_back(as_str_or_throw(feats[i], "input_features[i]"));
    }
    agent_meta_.F = static_cast<int>(agent_meta_.input_features.size());

    // --- class mapping ---
    const YAML::Node c2t = agent_raw_manifest_["adc_class_mapping"]["class_to_therapy"];
    if (!c2t || !c2t.IsMap()) {
        throw std::runtime_error("adc_class_mapping.class_to_therapy missing or not a map");
    }

    agent_meta_.class_to_therapy.clear();

    for (auto it = c2t.begin(); it != c2t.end(); ++it) {
        const std::string k = it->first.as<std::string>();
        const YAML::Node v = it->second;

        int cls = -1;
        try { cls = std::stoi(k); }
        catch (...) { throw std::runtime_error("class_to_therapy key is not int string: '" + k + "'"); }

        if (!v.IsScalar()) {
            throw std::runtime_error("class_to_therapy['" + k + "'] is not scalar");
        }

        int therapy = -1;
        try { therapy = v.as<int>(); }
        catch (...) {
            try { therapy = std::stoi(v.as<std::string>()); }
            catch (...) { throw std::runtime_error("class_to_therapy['" + k + "'] not int: '" + v.Scalar() + "'"); }
        }

        agent_meta_.class_to_therapy[cls] = therapy;
    }

    agent_meta_.K = static_cast<int>(agent_meta_.class_to_therapy.size());

    if (agent_meta_.L <= 0 || agent_meta_.H <= 0 || agent_meta_.F <= 0) {
        throw std::runtime_error("Invalid manifest values (L/H/F).");
    }

    spdlog::info("ADC manifest loaded: L={} H={} F={} K(map_size)={}",
                 agent_meta_.L, agent_meta_.H, agent_meta_.F, agent_meta_.K);
}

void AdaptiveCyclingAgent::initialize() {
    load_manifest();
    load_model();

    const auto* admin_mgr = Model::get_spatial_data()->get_admin_level_manager();
    const int admin_level_count = static_cast<int>(admin_mgr->get_level_names().size());
    const int n_levels_including_cell = admin_level_count + 1;

    adc_agent_data_by_level.resize(n_levels_including_cell);

    // Build therapy -> strategy mapping from configuration (AgentParameters.adc_agent.strategy_cycle)
    const auto strategy_cycle = Model::get_config()->get_agent_parameters().get_adc_agent().get_strategy_cycle();
    therapy_strategy_map_.clear();
    therapy_cycle_index_.clear();
    if (!strategy_cycle.empty()) {
        // Map for therapies 6,7,8 using provided strategy ids in order
        const std::vector<int> therapy_ids = {6,7,8};
        for (size_t i = 0; i < therapy_ids.size(); ++i) {
            // Each therapy gets the full cycle list
            therapy_strategy_map_[therapy_ids[i]] = strategy_cycle;
            therapy_cycle_index_[therapy_ids[i]] = 0;
        }
        spdlog::info("[ADC] Loaded therapy->strategy mapping from config: size={} ", strategy_cycle.size());
        for (const auto &kv : therapy_strategy_map_)
            spdlog::info("[ADC] therapy {} -> strategy cycle size {}", kv.first, kv.second.size());
    } else {
        // fallback to existing defaults (backwards compatibility)
        therapy_strategy_map_[6] = {0};
        therapy_strategy_map_[7] = {0};
        therapy_strategy_map_[8] = {0};
        spdlog::info("[ADC] No strategy_cycle in config; using default therapy->strategy map");
    }

    // Load trigger parameters
    const auto &adc_cfg = Model::get_config()->get_agent_parameters().get_adc_agent();
    trigger_value_ = adc_cfg.get_trigger_value();
    const date::year_month_day adc_trigger_ymd = adc_cfg.get_trigger_date();
    // prepare formatted trigger date string (YYYY/MM/DD) using date::format
    const std::string adc_date_str = date::format("%Y/%m/%d", adc_trigger_ymd);
    // Compute trigger_day_ as number of days from simulation starting_date to adc_trigger_ymd
    const auto starting_date = Model::get_config()->get_simulation_timeframe().get_starting_date();
    const auto sim_start_sys_days = date::sys_days{starting_date};
    const auto adc_trigger_sys_days = date::sys_days{adc_trigger_ymd};
    const auto dur = adc_trigger_sys_days - sim_start_sys_days;
    trigger_day_ = dur.count();
    if (trigger_day_ < 0) {
        spdlog::error("[ADC] adc.trigger_date {} is before simulation starting_date", adc_date_str);
        throw std::runtime_error("ADC trigger_date is before simulation starting_date");
    }
    trigger_month_ = static_cast<int>(trigger_day_) / 30;
    // Convert trigger_day_ to months (approx 30 days per month) for comparison with agent_meta_.L
    if (trigger_month_ < agent_meta_.L) {
        spdlog::error("[ADC] adc.trigger_date {} results in trigger_months={} which is less than model window L={}", adc_date_str, trigger_month_, agent_meta_.L);
        throw std::runtime_error("ADC trigger_date requires more history (increase simulation start or adjust trigger_date)");
    }

    spdlog::info("[ADC] trigger_value={} trigger_date={} trigger_month_={} months since start", trigger_value_, adc_date_str, trigger_month_);

    const int roll_window = 12;

    // explicit cap: need enough months to reach trigger_month_ and still compute rolling
    const int user_cap = agent_meta_.L + roll_window + 5; // buffer

    for (int level_id = 0; level_id < n_levels_including_cell; ++level_id) {
        const bool is_cell_level = (level_id == admin_level_count);
        if (is_cell_level && !(admin_level_count == 0)) {
            continue;
        }

        int vector_size = 0;
        if (is_cell_level) {
            vector_size = Model::get_config()->number_of_locations();
        } else {
            const auto* boundary = admin_mgr->get_boundary(admin_mgr->get_level_names()[level_id]);
            vector_size = boundary ? (boundary->max_unit_id + 1) : 0;
        }

        if (vector_size <= 0) {
            spdlog::warn("ADC init: level {} has invalid vector_size={}, skipping.", level_id, vector_size);
            continue;
        }
        auto& adc = adc_agent_data_by_level[level_id];

        adc.reset_month(vector_size);
        adc.ensure_adc_state_size_and_init(
            vector_size,
            0.1,
            365,
            365,
            {6,7,8}
        );
        adc.set_history_cap(agent_meta_.L, roll_window, user_cap);
        // NOTE: pass agent_meta_.L as the model window if set_history_cap interprets arg1 as "model window"
    }
    spdlog::info("[ADC] history cap set explicitly: cap={} (L={} trigger_month={} roll_window={})",
                 user_cap, agent_meta_.L, trigger_month_, roll_window);

    // Set SIM_START_YEAR and SIM_START_MONTH from starting_date
    this->sim_start_year_ = static_cast<int>(starting_date.year());
    this->sim_start_month_ = 0; // per your request set month to 0

    // Validate strategy ids exist in current strategy_db
    const auto &strategy_db = Model::get_strategy_db();
    for (auto it = therapy_strategy_map_.begin(); it != therapy_strategy_map_.end(); ) {
        const auto& cycle = it->second;
        bool all_valid = true;
        for (int strat : cycle) {
            if (strat < 0 || strat >= (int)strategy_db.size()) {
                spdlog::error("[ADC] Configured strategy_id {} for therapy {} is invalid (strategy_db size={}), removing mapping.", strat, it->first, strategy_db.size());
                all_valid = false;
                break;
            }
        }
        if (!all_valid) {
            it = therapy_strategy_map_.erase(it);
        } else {
            ++it;
        }
    }

    if (therapy_strategy_map_.empty()) {
        spdlog::warn("[ADC] therapy_strategy_map_ is empty after validation; ADC will not schedule strategy changes.");
    }
}

void AdaptiveCyclingAgent::monthly_collect_data_by_level(int level_id) {
    adc_agent_data_by_level[level_id].snapshot_adc_state_to_features();
}

void AdaptiveCyclingAgent::reset_adc_data(int level_id, int vector_size) {
    auto& d = adc_agent_data_by_level[level_id];
    d.ensure_guard_size(vector_size);   // keep guards persistent
    d.reset_month(vector_size);         // reset only monthly feature vectors
}

// ============================================================
// Refactor: finalize_month_580Y_freq() now calls inference_from_adc_data()
// ============================================================
void AdaptiveCyclingAgent::finalize_month_580Y_freq(
    int level_id,
    int numGenotypes,
    std::vector<SQLiteValidationReporter::MonthlyGenomeData> monthly_genome_data_by_level
) {
    auto& data = adc_agent_data_by_level[level_id];
    const int n_units = static_cast<int>(data.current_580Y_freq_by_unit.size());

    // ---- compute current 580Y freq per unit ----
    for (int unit_id = 0; unit_id < n_units; ++unit_id) {
        double total_w = 0.0;
        double y_w = 0.0;

        for (int g = 0; g < numGenotypes; ++g) {
            const double w = monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][g];
            total_w += w;

            if (Model::get_genotype_db()->is_matched_genotype_by_id(g, 12, 10, 'Y')) {
                y_w += w;
            }
        }

        data.current_580Y_freq_by_unit[unit_id] = (total_w > 0.0) ? (y_w / total_w) : 0.0;
    }

    // ---- snapshot -> push into history ----
    data.snapshot_adc_state_to_features();
    data.push_month_all();

    // spdlog::info("[ADC] level={} history_len={} need(L)={} now_day={} trigger_day={}",
    //          level_id, data.history_len(), agent_meta_.L,
    //          Model::get_scheduler()->current_time(), trigger_day_);


    // ---- Run inference when enough history ----
    const int now_day = Model::get_scheduler()->current_time();
    const bool enough_history = (data.history_len() >= agent_meta_.L);
    const bool past_trigger_date = (now_day >= trigger_day_);

    if (enough_history && past_trigger_date) {
        std::vector<int> next_month_therapy_by_unit;
        inference_from_adc_data(level_id, next_month_therapy_by_unit);

        // // 2) (optional) log next-month therapy for first few units
        // const int showN = std::min(3, (int)next_month_therapy_by_unit.size());
        // for (int u = 0; u < showN; ++u) {
        //     spdlog::info("[ADC DECISION] level={} unit={} next_month_therapy={}",
        //                  level_id, u, next_month_therapy_by_unit[u]);
        // }
        //
        // // 3) keep your existing debug if you still want it (but note: your old debug assumed single logits tensor)
        // //    If you want, I can rewrite inference_from_adc_data_debug to match the 2-head model as well.
        // std::vector<ADCInferenceDebug> dbg;
        // inference_from_adc_data_debug(level_id, dbg, /*compute_softmax=*/true);
        //
        // spdlog::info("[ADC DEBUG] level={} dbg size={}", level_id, dbg.size());
        // for (const auto& d : dbg) {
        //     if (d.unit_id < 3) {
        //         log_adc_inference_debug(d, agent_meta_.input_features);
        //     }
        // }
    }
    // else {
    //     spdlog::info("[ADC] skip inference: history_len={} (need L={}) now_day={} trigger_day={}",
    //                  data.history_len(), agent_meta_.L, now_day, trigger_day_);
    // }
}

// ============================================================
// Main inference: uses (exec_logits, excc_logits)
// output_classes[u] = therapy id for NEXT MONTH if exec prob >= thr else -1
// Also prints a readable 36-month forecast for first few units.
// ============================================================
void AdaptiveCyclingAgent::inference_from_adc_data(int level_id,
                                                   std::vector<int>& output_classes) {
    auto& data = adc_agent_data_by_level[level_id];

    const int L = agent_meta_.L;
    const int F = agent_meta_.F;

    const int now_day = Model::get_scheduler()->current_time();

    // Choose cooldown length here
    const int cooldown_days = 365 * 2;   // 2 years (use 365 for 1 year)

    const int end_month_exclusive = data.history_len();
    if (end_month_exclusive < L) {
        output_classes.clear();
        return;
    }

    if (now_day < trigger_day_) {
        output_classes.clear();
        return;
    }


    const std::vector<std::vector<float>> input_data =
        data.build_input_batch(end_month_exclusive, L, agent_meta_.input_features);

    const int N = static_cast<int>(input_data.size());

    data.ensure_guard_size(N);

    if (N == 0) {
        output_classes.clear();
        return;
    }
    if ((int)input_data[0].size() != L * F) {
        throw std::runtime_error("Input data shape mismatch from ADCAgentData (expected L*F).");
    }

    // -----------------------------
    // GLOBAL guard slot (since strategy switch is global)
    // -----------------------------
    const int GU = 0; // store global pending/cooldown in index 0

    // If pending switch day has passed, assume it executed, clear pending and start post-switch cooldown
    if (data.pending_switch_day_by_unit[GU] != -1 && now_day >= data.pending_switch_day_by_unit[GU]) {
        data.pending_switch_day_by_unit[GU]      = -1;
        data.pending_switch_therapy_by_unit[GU]  = -1;
        data.pending_switch_strategy_by_unit[GU] = -1;

        // Post-switch cooldown (prevents immediate re-scheduling right after a switch)
        data.block_inference_until_day_by_unit[GU] = now_day + cooldown_days;
    }

    // If still in cooldown, DO NOT schedule or run inference decision logic,
    // but we still allow output_classes to be computed (optional).
    const bool in_cooldown =
    (data.block_inference_until_day_by_unit[GU] != -1 && now_day < data.block_inference_until_day_by_unit[GU]);
    const bool has_future_pending =
        (data.pending_switch_day_by_unit[GU] != -1 && data.pending_switch_day_by_unit[GU] > now_day);

    if (in_cooldown || has_future_pending) {
        // still collecting history elsewhere, but no inference/printing now
        output_classes.clear();
        return;
    }


    // -----------------------------
    // Build x: (N, L, F)
    // -----------------------------
    torch::Tensor x = torch::zeros({N, L, F}, torch::kFloat32);
    for (int i = 0; i < N; ++i) {
        if ((int)input_data[i].size() != L * F) {
            throw std::runtime_error("Inconsistent row size in input_data from ADCAgentData.");
        }
        for (int j = 0; j < L * F; ++j) {
            const int f = j % F;
            const int t = j / F;
            x[i][t][f] = input_data[i][j];
        }
    }

    x = x.to(agent_model_.device);
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(x);

    // forward: tuple outputs
    ADCOutputs outs = forward_adc_outputs(agent_model_.module, inputs);
    torch::Tensor exec_logits = outs.exec_logits; // (N,H)
    torch::Tensor excc_logits = outs.excc_logits; // (N,H,K)

    if (exec_logits.dim() != 2 || exec_logits.size(0) != N) {
        throw std::runtime_error("exec_logits has unexpected shape (expected [N,H]).");
    }
    if (excc_logits.dim() != 3 || excc_logits.size(0) != N || excc_logits.size(1) != exec_logits.size(1)) {
        throw std::runtime_error("excc_logits has unexpected shape (expected [N,H,K]).");
    }

    const int H = (int)exec_logits.size(1);
    const int K_model = (int)excc_logits.size(2);

    // class->therapy vector sized to model K
    std::vector<int> class_to_therapy_vec(K_model, -1);
    for (const auto& kv : agent_meta_.class_to_therapy) {
        if (kv.first >= 0 && kv.first < K_model) {
            class_to_therapy_vec[kv.first] = kv.second;
        }
    }

    // parameters for readable output
    const float EXEC_THRESHOLD = (float)trigger_value_;  // from config
    const int tcur_month = end_month_exclusive - 1;

    output_classes.clear();
    output_classes.reserve(N);

    // print schedule for first few units
    const int units_to_print = std::min(3, N);
    const int days_per_month = 30;

    // -----------------------------
    // First pass: output_classes (per unit)
    // -----------------------------
    for (int u = 0; u < N; ++u) {
        // next month exec prob uses h=0
        float pe0 = sigmoidf(exec_logits[u][0].item<float>());

        int therapy_out = -1;
        if (pe0 >= EXEC_THRESHOLD) {
            int cls0 = excc_logits[u][0].argmax(/*dim=*/0).item<int>();
            if (cls0 >= 0 && cls0 < K_model) therapy_out = class_to_therapy_vec[cls0];
        }
        output_classes.push_back(therapy_out);
    }

    // -----------------------------
    // Printing: build p_exec/p_cls for first few units
    // -----------------------------
    for (int u = 0; u < units_to_print; ++u) {
        std::vector<float> p_exec(H, 0.f);
        std::vector<std::vector<float>> p_cls(H, std::vector<float>(K_model, 0.f));

        for (int h = 0; h < H; ++h) {
            p_exec[h] = sigmoidf(exec_logits[u][h].item<float>());

            std::vector<float> logits_k(K_model);
            for (int k = 0; k < K_model; ++k) logits_k[k] = excc_logits[u][h][k].item<float>();

            std::vector<float> probs_k;
            softmax_row(logits_k, probs_k);
            for (int k = 0; k < K_model; ++k) p_cls[h][k] = probs_k[k];
        }

        spdlog::info("[ADC FORECAST] level={} unit={} tcur_month={} next_month_therapy={}",
                     level_id, u, tcur_month, output_classes[u]);

        // IMPORTANT: print_adc_schedule_top1 MUST NOT schedule any events
        print_adc_schedule_top1(
            tcur_month,
            now_day,
            H,
            p_exec,
            p_cls,
            class_to_therapy_vec,
            EXEC_THRESHOLD,
            this->sim_start_year_,
            this->sim_start_month_,
            days_per_month
        );
    }

    // -----------------------------
    // Scheduling decision (GLOBAL): do it ONCE using unit 0
    // -----------------------------

    // Build p_exec/p_cls for unit 0 (global decision)
    {
        const int u = 0;

        std::vector<float> p_exec(H, 0.f);
        std::vector<std::vector<float>> p_cls(H, std::vector<float>(K_model, 0.f));

        for (int h = 0; h < H; ++h) {
            p_exec[h] = sigmoidf(exec_logits[u][h].item<float>());

            std::vector<float> logits_k(K_model);
            for (int k = 0; k < K_model; ++k) logits_k[k] = excc_logits[u][h][k].item<float>();

            std::vector<float> probs_k;
            softmax_row(logits_k, probs_k);
            for (int k = 0; k < K_model; ++k) p_cls[h][k] = probs_k[k];
        }

        // ---- pick Top-1 horizon month based on exec prob ----
        int best_h = -1;
        float best_pe = -1.f;

        for (int h = 0; h < H; ++h) {
            float pe = p_exec[h];
            if (pe < EXEC_THRESHOLD) continue;
            if (pe > best_pe) { best_pe = pe; best_h = h; }
        }
        if (best_h < 0) {
            // fallback: just max exec
            for (int h = 0; h < H; ++h) {
                float pe = p_exec[h];
                if (pe > best_pe) { best_pe = pe; best_h = h; }
            }
        }
        if (best_h < 0) return;

        // ---- pick class at that horizon ----
        int best_k = -1;
        float best_pk = -1.f;
        for (int k = 0; k < K_model; ++k) {
            float pk = p_cls[best_h][k];
            if (pk > best_pk) { best_pk = pk; best_k = k; }
        }

        const int therapy = (best_k >= 0 ? class_to_therapy_vec[best_k] : -1);
        const int forecast_day = now_day + (best_h + 1) * days_per_month;

        if (therapy < 0 || forecast_day <= now_day) return;

        if (!therapy_strategy_map_.count(therapy)) {
            spdlog::warn("[ADC] therapy {} not in therapy_strategy_map, skipping schedule", therapy);
            return;
        }
        // Get the cycle and current index for this therapy
        auto& cycle = therapy_strategy_map_.at(therapy);
        int& idx = therapy_cycle_index_[therapy];
        if (cycle.empty()) {
            spdlog::error("[ADC] strategy cycle for therapy {} is empty", therapy);
            return;
        }
        int strategy_id = cycle[idx];
        // Advance and wrap the index
        idx = (idx + 1) % cycle.size();

        // schedule ONCE
        auto event = std::make_unique<ChangeTreatmentStrategyEvent>(strategy_id, forecast_day);
        event->set_executable(true);
        Model::get_scheduler()->schedule_population_event(std::move(event));

        spdlog::info("[ADC] Scheduled ChangeTreatmentStrategyEvent strategy_id={} at day={} (therapy={})",
                     strategy_id, forecast_day, therapy);

        // mark pending + immediate cooldown
        data.pending_switch_day_by_unit[GU]      = forecast_day;
        data.pending_switch_therapy_by_unit[GU]  = therapy;
        data.pending_switch_strategy_by_unit[GU] = strategy_id;

        data.block_inference_until_day_by_unit[GU] = now_day + cooldown_days;
        spdlog::info("[ADC] Enter cooldown until day={}", data.block_inference_until_day_by_unit[GU]);
    }
}


// ============================================================
// (Your debug path remains as-is below. NOTE: Your current debug
// uses forward_logits() single tensor; that is NOT correct for
// the 2-head model. Keeping it to match your pasted code.
// If you want, I can rewrite debug to print exec + class properly.
// ============================================================
void AdaptiveCyclingAgent::inference_from_adc_data_debug(
    int level_id,
    std::vector<ADCInferenceDebug>& debug_out,
    bool compute_softmax
) {
    auto& data = adc_agent_data_by_level[level_id];

    const int L = agent_meta_.L;
    const int F = agent_meta_.F;

    const int end_month_exclusive = data.history_len();
    if (end_month_exclusive < L) {
        debug_out.clear();
        return;
    }

    auto input_data = data.build_input_batch(end_month_exclusive, L, agent_meta_.input_features);

    const int N = static_cast<int>(input_data.size());
    if (N == 0) {
        debug_out.clear();
        return;
    }

    torch::Tensor x = torch::zeros({N, L, F}, torch::kFloat32);
    for (int u = 0; u < N; ++u) {
        for (int j = 0; j < L * F; ++j) {
            const int f = j % F;
            const int t = j / F;
            x[u][t][f] = input_data[u][j];
        }
    }

    x = x.to(agent_model_.device);

    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(x);

    // IMPORTANT: If your model is 2-head tuple, this debug should be updated.
    // Keeping this code as pasted; may not reflect exec/class correctly.
    torch::jit::IValue out = agent_model_.module.forward(inputs);
    torch::Tensor logits;

    if (out.isTensor()) {
        logits = out.toTensor();
    } else if (out.isTuple()) {
        auto tup = out.toTuple();
        const auto& elems = tup->elements();
        if (elems.empty() || !elems[0].isTensor()) {
            throw std::runtime_error("Debug forward returned tuple but first element not tensor.");
        }
        logits = elems[0].toTensor();
    } else {
        throw std::runtime_error("Debug forward returned unsupported type.");
    }

    if (logits.dim() != 2 || logits.size(0) != N) {
        throw std::runtime_error("ADC model output has unexpected shape (expected [N, K_out]).");
    }

    const int K_out = static_cast<int>(logits.size(1));

    torch::Tensor probs;
    if (compute_softmax) {
        probs = torch::softmax(logits, /*dim=*/1);
    }

    debug_out.clear();
    debug_out.reserve(N);

    for (int u = 0; u < N; ++u) {
        ADCInferenceDebug dbg;
        dbg.level_id = level_id;
        dbg.unit_id = u;
        dbg.end_month = end_month_exclusive;

        dbg.logits.resize(K_out);
        for (int k = 0; k < K_out; ++k) {
            dbg.logits[k] = logits[u][k].item<float>();
        }

        if (compute_softmax) {
            dbg.probabilities.resize(K_out);
            for (int k = 0; k < K_out; ++k) {
                dbg.probabilities[k] = probs[u][k].item<float>();
            }
        }

        dbg.predicted_class = logits[u].argmax(/*dim=*/0).item<int>();

        auto it = agent_meta_.class_to_therapy.find(dbg.predicted_class);
        dbg.predicted_therapy = (it != agent_meta_.class_to_therapy.end()) ? it->second : -1;

        dbg.input_last_timestep.resize(F);
        for (int f = 0; f < F; ++f) {
            dbg.input_last_timestep[f] = input_data[u][(L - 1) * F + f];
        }

        debug_out.push_back(std::move(dbg));
    }
}

void AdaptiveCyclingAgent::log_adc_inference_debug(
    const AdaptiveCyclingAgent::ADCInferenceDebug& d,
    const std::vector<std::string>& feature_names
) {
    spdlog::info("[ADC DEBUG] level={} unit={} month={} class={} therapy={}",
                 d.level_id, d.unit_id, d.end_month, d.predicted_class, d.predicted_therapy);

    if (!d.probabilities.empty()) {
        spdlog::info("  probabilities:");
        for (size_t k = 0; k < d.probabilities.size(); ++k) {
            spdlog::info("    [{}] {}", k, d.probabilities[k]);
        }
    }

    if (d.input_last_timestep.size() == feature_names.size()) {
        spdlog::info("  input[t=-1]:");
        for (size_t i = 0; i < feature_names.size(); ++i) {
            spdlog::info("    {} = {}", feature_names[i], d.input_last_timestep[i]);
        }
    } else {
        spdlog::warn("  input[t=-1] size mismatch: got {}, expected {}",
                     d.input_last_timestep.size(), feature_names.size());
    }
}

// ============================================================
// Feature batch builder (your mapping rules)
// ============================================================
std::vector<std::vector<float>> AdaptiveCyclingAgent::ADCAgentData::build_input_batch(
    int end_month_exclusive,
    int L,
    const std::vector<std::string>& feature_names
) const {
    const int T = history_len();
    if (L <= 0) throw std::runtime_error("build_input_batch: L must be > 0");
    if (end_month_exclusive <= 0) throw std::runtime_error("build_input_batch: end_month_exclusive must be > 0");
    if (end_month_exclusive > T) {
        throw std::runtime_error(
            "build_input_batch: end_month_exclusive=" + std::to_string(end_month_exclusive) +
            " exceeds history_len=" + std::to_string(T)
        );
    }
    if (end_month_exclusive < L) {
        throw std::runtime_error(
            "build_input_batch: need end_month_exclusive >= L. end=" +
            std::to_string(end_month_exclusive) + " L=" + std::to_string(L)
        );
    }

    // Infer number of units from any stored history vector
    int n_units = 0;
    if (!current_tf_hist.empty()) n_units = (int)current_tf_hist.back().size();
    else if (!freq_580Y_hist.empty()) n_units = (int)freq_580Y_hist.back().size();
    else {
        throw std::runtime_error("build_input_batch: no history available to infer n_units");
    }

    const int F = (int)feature_names.size();
    const int start_t = end_month_exclusive - L;

    auto get_feat = [&](const std::string& name, int t, int u) -> double {

        // 1) Map manifest feature name -> internal key (EXACT mapping you requested)
        const std::string key = [&]() -> std::string {
            if (name == "current_TF_by_location") return "current_tf_by_unit";
            if (name == "monthly_number_of_TF_by_location") return "monthly_number_of_tf_by_unit";
            if (name == "cumulative_TF_by_location") return "accumulative_tf_by_unit";
            if (name == "cumulative_NTF_by_location") return "accumulative_ntf_by_unit";
            if (name == "tf_total") return "monthly_number_of_tf_by_unit"; // your rule
            return name;
        }();

        // ---- TF totals ----
        if (key == "current_tf_by_unit")            return current_tf_hist[t][u];
        if (key == "monthly_number_of_tf_by_unit")  return monthly_tf_hist[t][u];
        if (key == "accumulative_tf_by_unit")       return acc_tf_hist[t][u];
        if (key == "accumulative_ntf_by_unit")      return acc_ntf_hist[t][u];

        // ---- therapy TF ----
        if (key == "tf_by_therapy_6") return tf6_hist[t][u];
        if (key == "tf_by_therapy_7") return tf7_hist[t][u];
        if (key == "tf_by_therapy_8") return tf8_hist[t][u];

        auto therapy_sum = [&]() -> double {
            const double a = tf6_hist[t][u];
            const double b = tf7_hist[t][u];
            const double c = tf8_hist[t][u];
            return a + b + c;
        };

        // ---- tf_by_therapy_total: normalized sum [0,1] ----
        if (key == "tf_by_therapy_total") {
            const double s = therapy_sum();
            const double denom = std::max(s, 1.0);
            return s / denom;
        }

        // ---- therapy shares ----
        if (key == "therapy_share_0") {
            const double denom = std::max(therapy_sum(), 1.0);
            return tf6_hist[t][u] / denom;
        }
        if (key == "therapy_share_1") {
            const double denom = std::max(therapy_sum(), 1.0);
            return tf7_hist[t][u] / denom;
        }
        if (key == "therapy_share_2") {
            const double denom = std::max(therapy_sum(), 1.0);
            return tf8_hist[t][u] / denom;
        }

        // ---- entropy ----
        if (key == "therapy_usage_entropy") {
            constexpr double eps = 1e-9;
            const double denom = std::max(therapy_sum(), 1.0);

            const double p0 = tf6_hist[t][u] / denom;
            const double p1 = tf7_hist[t][u] / denom;
            const double p2 = tf8_hist[t][u] / denom;

            auto h = [&](double p) -> double {
                const double pp = std::max(p, eps);
                return -p * std::log(pp);
            };
            return h(p0) + h(p1) + h(p2);
        }

        // ---- 580Y ----
        if (key == "freq_580Y")             return freq_580Y_hist[t][u];
        if (key == "freq_580Y_delta")       return freq_580Y_delta(t, u);
        if (key == "freq_580Y_rollmean_12") return freq_580Y_rollmean_12(t, u);

        // ---- ADC ----
        if (key == "adc_deployed")      return adc_deployed_hist[t][u];
        if (key == "adc_trigger_value") return adc_trigger_value_hist[t][u];
        if (key == "adc_delay_days")    return adc_delay_days_hist[t][u];
        if (key == "adc_turn_off_days") return adc_turn_off_days_hist[t][u];

        spdlog::warn("[ADC] build_input_batch: unknown feature '{}', using 0", name);
        return 0.0;
    };

    std::vector<std::vector<float>> batch;
    batch.resize(n_units, std::vector<float>(L * F, 0.0f));

    for (int u = 0; u < n_units; ++u) {
        int idx = 0;
        for (int t = start_t; t < end_month_exclusive; ++t) {
            for (int f = 0; f < F; ++f) {
                const double v = get_feat(feature_names[f], t, u);
                batch[u][idx++] = (float)v;
            }
        }
    }

    return batch;
}

// ============================================================
// Printable forecast
// ============================================================
void AdaptiveCyclingAgent::print_adc_schedule(
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
) {
    const int K = (int)class_to_therapy.size();
    const int base_month = tcur_month + 1;   // next month index
    const int base_day   = tcur_day;         // current day at "now" (end of current month)

    spdlog::info("[ADC FORECAST] Next {} months (from month={} day={})",
                 horizon, base_month, base_day);

    for (int h = 0; h < horizon; ++h) {
        const float pe = p_exec[h];
        if (pe < exec_threshold) continue;

        // best class
        int best_k = -1;
        float best_p = -1.f;
        for (int k = 0; k < K; ++k) {
            if (p_cls[h][k] > best_p) {
                best_p = p_cls[h][k];
                best_k = k;
            }
        }

        const int therapy = (best_k >= 0 ? class_to_therapy[best_k] : -1);

        // month/date label
        YearMonth ym = add_months(sim_start_year, sim_start_month, base_month + h);

        // NEW: forecast day for this horizon step
        const int forecast_day = base_day + (h + 1) * days_per_month;

        spdlog::info(
            "  {} | +{:02d}m | day={} | EXEC p={:.3f} -> therapy {} (cls_p={:.3f})",
            ym_to_string(ym), h + 1, forecast_day, pe, therapy, best_p
        );
    }
}


AdaptiveCyclingAgent::YearMonth AdaptiveCyclingAgent::add_months(int start_year, int start_month, int offset) {
    YearMonth result;
    int m0 = (start_month - 1) + offset;
    result.year = start_year + (m0 / 12);
    result.month = (m0 % 12) + 1;
    return result;
}

std::string AdaptiveCyclingAgent::ym_to_string(const AdaptiveCyclingAgent::YearMonth& ym) {
    std::ostringstream os;
    os << ym.year << "-" << std::setw(2) << std::setfill('0') << ym.month;
    return os.str();
}

void AdaptiveCyclingAgent::print_adc_schedule_top1(
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
) {
    const int K = (int)class_to_therapy.size();
    const int base_month = tcur_month + 1;
    const int base_day   = tcur_day;

    // 1) find best month by exec prob (optionally thresholded)
    int best_h = -1;
    float best_pe = -1.f;

    for (int h = 0; h < horizon; ++h) {
        const float pe = p_exec[h];
        if (pe < exec_threshold) continue; // remove this line if you always want top1 even if small
        if (pe > best_pe) {
            best_pe = pe;
            best_h = h;
        }
    }

    // If nothing passes threshold, you can either print nothing or fall back to global max.
    if (best_h < 0) {
        // fallback: global max (no threshold)
        for (int h = 0; h < horizon; ++h) {
            const float pe = p_exec[h];
            if (pe > best_pe) {
                best_pe = pe;
                best_h = h;
            }
        }
    }
    if (best_h < 0) {
        spdlog::info("[ADC FORECAST] No forecast months available.");
        return;
    }

    // 2) pick therapy at best_h
    int best_k = -1;
    float best_cls_p = -1.f;
    for (int k = 0; k < K; ++k) {
        const float pk = p_cls[best_h][k];
        if (pk > best_cls_p) {
            best_cls_p = pk;
            best_k = k;
        }
    }

    const int therapy = (best_k >= 0 ? class_to_therapy[best_k] : -1);

    // 3) compute date labels
    YearMonth ym = add_months(sim_start_year, sim_start_month, base_month + best_h);
    const int forecast_day = base_day + (best_h + 1) * days_per_month;

    spdlog::info("[ADC FORECAST] Top-1 event in next {} months (from month={} day={})",
                 horizon, base_month, base_day);

    spdlog::info(
        "  {} | +{:02d}m | day={} | EXEC p={:.3f} -> therapy {} (cls_p={:.3f})",
        ym_to_string(ym), best_h + 1, forecast_day, best_pe, therapy, best_cls_p
    );
}

static AdaptiveCyclingAgent::ADCTop1Decision compute_top1(
    int tcur_month, int tcur_day, int horizon,
    const std::vector<float>& p_exec,
    const std::vector<std::vector<float>>& p_cls,
    const std::vector<int>& class_to_therapy,
    float exec_threshold,
    int days_per_month
) {
    AdaptiveCyclingAgent::ADCTop1Decision d;

    const int K = (int)class_to_therapy.size();
    const int base_month = tcur_month + 1;
    const int base_day   = tcur_day;

    int best_h = -1;
    float best_pe = -1.f;
    for (int h = 0; h < horizon; ++h) {
        float pe = p_exec[h];
        if (pe < exec_threshold) continue;
        if (pe > best_pe) { best_pe = pe; best_h = h; }
    }
    if (best_h < 0) { // fallback: global max
        for (int h = 0; h < horizon; ++h) {
            float pe = p_exec[h];
            if (pe > best_pe) { best_pe = pe; best_h = h; }
        }
    }
    if (best_h < 0) return d;

    int best_k = -1;
    float best_cls_p = -1.f;
    for (int k = 0; k < K; ++k) {
        float pk = p_cls[best_h][k];
        if (pk > best_cls_p) { best_cls_p = pk; best_k = k; }
    }

    d.best_h = best_h;
    d.exec_p = best_pe;
    d.cls_p  = best_cls_p;
    d.therapy = (best_k >= 0 ? class_to_therapy[best_k] : -1);
    d.forecast_day = base_day + (best_h + 1) * days_per_month;

    return d;
}

