#include "AdaptiveCyclingAgent.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <date/date.h>

#include "Events/Population/ChangeTreatmentStrategyEvent.h"
#include "Parasites/Genotype.h"
#include "Reporters/SQLiteValidationReporter.h"
#include "Simulation/Model.h"

// ============================================================
// AdaptiveCyclingAgent
// ============================================================

AdaptiveCyclingAgent::AdaptiveCyclingAgent() = default;

// ============================================================
// Static helpers
// ============================================================

int AdaptiveCyclingAgent::as_int_or_throw(const YAML::Node& n, const char* path) {
    if (!n || !n.IsScalar())
        throw std::runtime_error(std::string("Missing or non-scalar YAML node: ") + path);
    try { return n.as<int>(); }
    catch (...) { throw std::runtime_error(std::string("Cannot parse int from YAML node: ") + path); }
}

std::string AdaptiveCyclingAgent::as_str_or_throw(const YAML::Node& n, const char* path) {
    if (!n || !n.IsScalar())
        throw std::runtime_error(std::string("Missing or non-scalar YAML node: ") + path);
    return n.as<std::string>();
}

// Majority-vote smoothing over a rolling window of raw argmax values
int AdaptiveCyclingAgent::smoothed_therapy(SwitchState& sw, int raw_argmax) {
    constexpr int SMOOTH_WIN = 3;
    sw.raw_buf.push_back(raw_argmax);
    if (static_cast<int>(sw.raw_buf.size()) > SMOOTH_WIN)
        sw.raw_buf.erase(sw.raw_buf.begin());
    int counts[3] = {0, 0, 0};
    for (int v : sw.raw_buf)
        if (v >= 0 && v < 3) ++counts[v];
    return static_cast<int>(std::max_element(counts, counts + 3) - counts);
}

// Update switch state after a therapy decision at absolute month t_pb
void AdaptiveCyclingAgent::update_switch_state(SwitchState& sw, int t_pb, int sm, float tf) {
    if (sw.prev_smoothed >= 0 && sm != sw.prev_smoothed) {
        if (sw.last_switch_t >= 0) {
            const int interval = t_pb - sw.last_switch_t;
            if (interval > 0) {
                sw.n_intervals++;
                sw.mean_interval += (static_cast<float>(interval) - sw.mean_interval)
                                     / static_cast<float>(sw.n_intervals);
            }
        }
        sw.last_switch_t = t_pb;
        sw.tf_at_switch  = tf;
    }
    sw.prev_smoothed = sm;
}

// Build the 3 switch-timing features (f[50], f[51], f[52])
std::array<float, 3> AdaptiveCyclingAgent::switch_feats(int t_pb, const SwitchState& sw) {
    const float months_since = (sw.last_switch_t >= 0)
                               ? static_cast<float>(t_pb - sw.last_switch_t) / 24.f
                               : 0.f;
    return { months_since, sw.tf_at_switch, sw.mean_interval / 24.f };
}

// ============================================================
// load_manifest
// ============================================================

void AdaptiveCyclingAgent::load_manifest() {
    const auto manifest_path =
        Model::get_config()->get_agent_parameters().get_adc_agent().get_manifest_path();

    spdlog::info("[ADC] Loading manifest from: {}", manifest_path);
    agent_raw_manifest_ = ModelLoader::load_manifest(manifest_path);

    // --- window W ---
    agent_meta_.W = as_int_or_throw(agent_raw_manifest_["window"]["W"], "window.W");

    // --- input_features ---
    const YAML::Node feats = agent_raw_manifest_["input_features"];
    if (!feats || !feats.IsSequence())
        throw std::runtime_error("[ADC] input_features missing or not a sequence in manifest");
    agent_meta_.input_features.clear();
    agent_meta_.input_features.reserve(feats.size());
    for (std::size_t i = 0; i < feats.size(); ++i)
        agent_meta_.input_features.push_back(as_str_or_throw(feats[i], "input_features[i]"));
    agent_meta_.F = static_cast<int>(agent_meta_.input_features.size());

    if (agent_meta_.W <= 0 || agent_meta_.F <= 0)
        throw std::runtime_error("[ADC] Invalid manifest: W or F <= 0");

    // --- allele_patterns (override defaults from YAML if present) ---
    const YAML::Node ap = agent_raw_manifest_["allele_patterns"];
    if (ap && ap.IsMap()) {
        const std::array<const char*, 4> keys = {"ART", "PPQ", "LUM", "AMQ"};
        for (int i = 0; i < 4; ++i) {
            if (ap[keys[i]] && ap[keys[i]].IsScalar())
                allele_patterns_[i] = ap[keys[i]].as<std::string>();
        }
    }

    // --- inference constants (optional overrides) ---
    const YAML::Node inf = agent_raw_manifest_["inference"];
    if (inf && inf["switch_threshold"] && inf["switch_threshold"].IsScalar())
        trigger_value_ = inf["switch_threshold"].as<double>();

    spdlog::info("[ADC] Manifest loaded: W={} F={}", agent_meta_.W, agent_meta_.F);
    spdlog::info("[ADC] Allele patterns: ART='{}' PPQ='{}' LUM='{}' AMQ='{}'",
                 allele_patterns_[0], allele_patterns_[1],
                 allele_patterns_[2], allele_patterns_[3]);
}

// ============================================================
// load_model
// ============================================================

void AdaptiveCyclingAgent::load_model() {
    const auto model_path =
        Model::get_config()->get_agent_parameters().get_adc_agent().get_model_path();

    agent_raw_model_ = ModelLoader::load_model(model_path);
    agent_raw_model_.eval();

    agent_model_ = ADCTorchModel{
        .meta   = agent_meta_,
        .module = agent_raw_model_,
        .device = torch::kCPU
    };

    spdlog::info("[ADC] Model loaded: {}", model_path);
}

// ============================================================
// initialize
// ============================================================

void AdaptiveCyclingAgent::initialize() {
    load_manifest();
    load_model();

    const auto* admin_mgr        = Model::get_spatial_data()->get_admin_level_manager();
    const int   admin_level_count = static_cast<int>(admin_mgr->get_level_names().size());
    const int   n_levels          = admin_level_count + 1;   // +1 for cell level

    adc_agent_data_by_level.resize(n_levels);

    // --- therapy -> strategy mapping from config strategy_cycle ---
    // strategy_cycle order: [strat_for_th8, strat_for_th7, strat_for_th6]
    const auto strategy_cycle =
        Model::get_config()->get_agent_parameters().get_adc_agent().get_strategy_cycle();

    therapy_to_strategy_.clear();
    if (strategy_cycle.size() >= 2) {
        therapy_to_strategy_[8] = strategy_cycle[0];
        therapy_to_strategy_[7] = strategy_cycle[1];
        therapy_to_strategy_[6] = strategy_cycle.size() >= 3 ? strategy_cycle[2] : strategy_cycle[0];
        spdlog::info("[ADC] therapy_to_strategy: 6->{}  7->{}  8->{}",
                     therapy_to_strategy_[6], therapy_to_strategy_[7], therapy_to_strategy_[8]);
    } else if (!strategy_cycle.empty()) {
        therapy_to_strategy_[6] = strategy_cycle[0];
        therapy_to_strategy_[7] = strategy_cycle[0];
        therapy_to_strategy_[8] = strategy_cycle[0];
        spdlog::info("[ADC] therapy_to_strategy: all->{}", strategy_cycle[0]);
    } else {
        spdlog::warn("[ADC] No strategy_cycle in config; therapy_to_strategy_ will be empty");
    }

    // Validate strategy ids against strategy_db
    const auto& strategy_db = Model::get_strategy_db();
    for (auto it = therapy_to_strategy_.begin(); it != therapy_to_strategy_.end(); ) {
        const int sid = it->second;
        if (sid < 0 || sid >= static_cast<int>(strategy_db.size())) {
            spdlog::error("[ADC] strategy_id={} for therapy={} invalid (strategy_db.size={}), removing",
                          sid, it->first, strategy_db.size());
            it = therapy_to_strategy_.erase(it);
        } else {
            ++it;
        }
    }
    if (therapy_to_strategy_.empty())
        spdlog::warn("[ADC] therapy_to_strategy_ empty after validation; no strategy changes will fire");

    // --- trigger parameters ---
    const auto& adc_cfg       = Model::get_config()->get_agent_parameters().get_adc_agent();
    trigger_value_             = adc_cfg.get_trigger_value();
    const auto adc_trigger_ymd = adc_cfg.get_trigger_date();
    const auto starting_date   = Model::get_config()->get_simulation_timeframe().get_starting_date();

    const std::string adc_date_str = date::format("%Y/%m/%d", adc_trigger_ymd);
    trigger_day_   = static_cast<long>(
        (date::sys_days{adc_trigger_ymd} - date::sys_days{starting_date}).count());
    trigger_month_ = static_cast<int>(trigger_day_) / 30;

    if (trigger_day_ < 0)
        throw std::runtime_error("[ADC] trigger_date is before simulation starting_date");
    if (trigger_month_ < agent_meta_.W)
        throw std::runtime_error("[ADC] trigger_date requires more history; increase it by >= W months");

    // --- beta_norm ---
    const YAML::Node inf = agent_raw_manifest_["inference"];
    double beta_min = 0.089, beta_max = 0.630;
    if (inf) {
        if (inf["beta_min"] && inf["beta_min"].IsScalar()) beta_min = inf["beta_min"].as<double>();
        if (inf["beta_max"] && inf["beta_max"].IsScalar()) beta_max = inf["beta_max"].as<double>();
    }
    const double beta_val = Model::get_config()->location_db()[0].beta;
    beta_norm_ = static_cast<float>(
        (beta_max > beta_min)
        ? std::clamp((beta_val - beta_min) / (beta_max - beta_min), 0.0, 1.0)
        : 0.5);

    // --- simulation start ---
    sim_start_year_  = static_cast<int>(starting_date.year());
    sim_start_month_ = 0;

    spdlog::info("[ADC] trigger_value={} trigger_date={} trigger_month={} beta_norm={:.4f}",
                 trigger_value_, adc_date_str, trigger_month_, beta_norm_);

    // --- per-level ADCAgentData init ---
    for (int level_id = 0; level_id < n_levels; ++level_id) {
        const bool is_cell_level = (level_id == admin_level_count);
        if (is_cell_level && admin_level_count != 0) continue;

        int vector_size = 0;
        if (is_cell_level) {
            vector_size = Model::get_config()->number_of_locations();
        } else {
            const auto* bnd = admin_mgr->get_boundary(admin_mgr->get_level_names()[level_id]);
            vector_size = bnd ? (bnd->max_unit_id + 1) : 0;
        }
        if (vector_size <= 0) {
            spdlog::warn("[ADC] level={} vector_size={}, skipping", level_id, vector_size);
            continue;
        }

        auto& adc = adc_agent_data_by_level[level_id];
        adc.set_history_cap(agent_meta_.W);
        adc.reset_month(vector_size);
        adc.ensure_state_size(vector_size);
    }

    spdlog::info("[ADC] Initialized: W={} F={} levels={}", agent_meta_.W, agent_meta_.F, n_levels);
}

// ============================================================
// reset_adc_data  (called from SQLiteValidationReporter each month)
// ============================================================

void AdaptiveCyclingAgent::reset_adc_data(int level_id, int vector_size) {
    auto& d = adc_agent_data_by_level[level_id];
    d.ensure_state_size(vector_size);   // guards / switch-state survive reset
    d.reset_month(vector_size);         // zero monthly accumulators
}

// ============================================================
// finalize_month_all_features
//   Replaces the old finalize_month_580Y_freq().
//   1. Computes ART/PPQ/LUM/AMQ allele freqs via regex-matched genotype IDs.
//   2. Pushes this month's data into all history deques.
//   3. Triggers inference when enough history and past the trigger date.
// ============================================================

void AdaptiveCyclingAgent::finalize_month_all_features(
    int level_id,
    int numGenotypes,
    const std::vector<SQLiteValidationReporter::MonthlyGenomeData>& monthly_genome_data_by_level
) {
    auto& data    = adc_agent_data_by_level[level_id];
    const int n_units = static_cast<int>(data.freq_ART.size());

    // ---- Build allele genotype-ID sets lazily; rebuild if genotype count changes ----
    if (!allele_genotype_ids_built_ || allele_genotype_ids_n_geno_ != numGenotypes) {
        auto* gdb = Model::get_genotype_db();
        for (int ai = 0; ai < 4; ++ai) {
            allele_genotype_ids_[ai].clear();
            const std::regex re(allele_patterns_[ai]);
            for (int g = 0; g < numGenotypes; ++g) {
                if (std::regex_match(gdb->at(g)->aa_sequence,re))
                    allele_genotype_ids_[ai].insert(g);
            }
            spdlog::debug("[ADC] allele[{}] pattern='{}' matched {} genotypes",
                          ai, allele_patterns_[ai],
                          static_cast<int>(allele_genotype_ids_[ai].size()));
        }
        allele_genotype_ids_built_  = true;
        allele_genotype_ids_n_geno_ = numGenotypes;
    }

    // ---- Compute allele frequencies per unit (one pass over weighted_occurrences) ----
    const auto& mgd = monthly_genome_data_by_level[level_id];

    for (int u = 0; u < n_units; ++u) {
        double total_w           = 0.0;
        std::array<double, 4> aw = {0.0, 0.0, 0.0, 0.0};

        for (int g = 0; g < numGenotypes; ++g) {
            const double w = mgd.weighted_occurrences[u][g];
            total_w += w;
            for (int ai = 0; ai < 4; ++ai)
                if (allele_genotype_ids_[ai].count(g))
                    aw[ai] += w;
        }

        const double denom = (total_w > 0.0) ? total_w : 1.0;
        data.freq_ART[u] = aw[0] / denom;
        data.freq_PPQ[u] = aw[1] / denom;
        data.freq_LUM[u] = aw[2] / denom;
        data.freq_AMQ[u] = aw[3] / denom;
    }

    // ---- Push this month's accumulators into all history deques ----
    data.push_month_all();

    // ---- Run inference when enough history has accumulated and past trigger date ----
    const int  now_day      = Model::get_scheduler()->current_time();
    const bool enough_hist  = (data.history_len() >= agent_meta_.W);
    const bool past_trigger = (now_day >= trigger_day_);

    if (enough_hist && past_trigger)
        inference_from_adc_data(level_id);
}

// ============================================================
// inference_from_adc_data
//
// Model TorchScript interface (predict_cpp):
//   Input:  Tensor (B, W=24, F=55)  float32
//   Output: Tuple(dist[B,3], switch_prob[B])
//     dist[:,0..2] = [d6, d7, d8]  Dirichlet expected, sums to 1
//     switch_prob  = sigmoid P(dominant therapy changes at T+24)
//
// Falls back to forward() if predict_cpp is unavailable; in that case the
// output may be a plain Tensor[B,3] (dist only) and switch_prob is derived
// as max(dist) per row.
// ============================================================

void AdaptiveCyclingAgent::inference_from_adc_data(int level_id) {
    auto& data    = adc_agent_data_by_level[level_id];
    const int W   = agent_meta_.W;
    const int F   = agent_meta_.F;
    const int now_day = Model::get_scheduler()->current_time();

    if (data.history_len() < W) return;
    if (now_day < trigger_day_)  return;

    // ---- Read inference constants from manifest ----
    const YAML::Node inf      = agent_raw_manifest_["inference"];
    const int burn_in         = inf && inf["burn_in"]         ? inf["burn_in"].as<int>()         : 120;
    const int t_active        = inf && inf["t_active"]        ? inf["t_active"].as<int>()        : 241;
    const int horizon_months  = inf && inf["horizon_months"]  ? inf["horizon_months"].as<int>()  : 24;
    const int cooldown_months = inf && inf["cooldown_months"] ? inf["cooldown_months"].as<int>() : 24;
    const int cooldown_days   = cooldown_months * 30;
    const float sw_threshold  = static_cast<float>(trigger_value_);

    // Absolute month index of the most-recently-pushed history step
    const int t_pb    = data.history_len() - 1;
    const int n_units = static_cast<int>(data.switch_state.size());
    if (n_units == 0) return;

    // ---- Global guard: unit 0 holds the simulation-wide cooldown ----
    const int GU = 0;
    {
        auto& g = data.guard[GU];
        // If a previously scheduled switch has now passed, clear it and start cooldown
        if (g.pending_day != -1 && now_day >= g.pending_day) {
            g.pending_day      = -1;
            g.pending_strategy = -1;
            g.block_until_day  = now_day + cooldown_days;
        }
        const bool in_cooldown = (g.block_until_day != -1 && now_day < g.block_until_day);
        const bool has_pending = (g.pending_day     != -1 && g.pending_day > now_day);
        if (in_cooldown || has_pending) return;
    }

    // ---- Build (N, W, F) input tensor ----
    const std::vector<std::vector<float>> input_batch =
        data.build_input_batch(W, F, beta_norm_, t_pb, burn_in, t_active);

    const int N = static_cast<int>(input_batch.size());
    if (N == 0) return;
    if (static_cast<int>(input_batch[0].size()) != W * F)
        throw std::runtime_error("[ADC] build_input_batch returned wrong row size");

    torch::Tensor x = torch::zeros({N, W, F}, torch::kFloat32);
    {
        auto acc = x.accessor<float, 3>();
        for (int i = 0; i < N; ++i)
            for (int t = 0; t < W; ++t)
                for (int f = 0; f < F; ++f)
                    acc[i][t][f] = input_batch[i][t * F + f];
    }
    x = x.to(agent_model_.device);

    // ---- Forward pass: try predict_cpp, fall back to forward() ----
    //
    // predict_cpp is the TorchScript-exported named method that returns
    // Tuple(dist[N,3], switch_prob[N]).  forward() returns only dist[N,3]
    // or the same tuple depending on the export.
    torch::NoGradGuard no_grad;
    torch::jit::IValue out;
    bool used_predict_cpp = false;

    try {
        out = agent_model_.module.run_method("predict_cpp", x);
        used_predict_cpp = true;
    } catch (const std::exception&) {
        try {
            std::vector<torch::jit::IValue> fwd_inputs{x};
            out = agent_model_.module.forward(fwd_inputs);
        } catch (const std::exception& e2) {
            spdlog::error("[ADC] Model forward failed: {}", e2.what());
            return;
        }
    }

    // ---- Unpack output ----
    // Handles all known TorchScript wrapping patterns:
    //   Tuple(Tensor[N,3], Tensor[N])     <- predict_cpp normal
    //   Tuple(Tuple(Tensor[N,3],Tensor[N]))  <- rare extra wrapper
    //   Tensor[N,3]                        <- forward-only export
    torch::Tensor dist, switch_prob;

    if (out.isTuple()) {
        const auto& elems = out.toTuple()->elements();

        // Extra outer wrapper: ((dist, sw),)
        if (elems.size() == 1 && elems[0].isTuple()) {
            const auto& inner = elems[0].toTuple()->elements();
            if (inner.size() >= 2 && inner[0].isTensor() && inner[1].isTensor()) {
                dist        = inner[0].toTensor().to(torch::kCPU).contiguous();
                switch_prob = inner[1].toTensor().to(torch::kCPU).contiguous();
            } else if (!inner.empty() && inner[0].isTensor()) {
                dist = inner[0].toTensor().to(torch::kCPU).contiguous();
            }
        }
        // Normal (dist, switch_prob)
        else if (elems.size() >= 2 && elems[0].isTensor() && elems[1].isTensor()) {
            dist        = elems[0].toTensor().to(torch::kCPU).contiguous();
            switch_prob = elems[1].toTensor().to(torch::kCPU).contiguous();
        }
        // Tuple with only dist
        else if (!elems.empty() && elems[0].isTensor()) {
            dist = elems[0].toTensor().to(torch::kCPU).contiguous();
        }
    } else if (out.isTensor()) {
        dist = out.toTensor().to(torch::kCPU).contiguous();
    }

    if (!dist.defined()) {
        spdlog::error("[ADC] Could not extract dist tensor from model output "
                      "(used_predict_cpp={})", used_predict_cpp);
        return;
    }

    // Validate dist shape [N, 3]
    if (dist.dim() != 2 || dist.size(0) != N || dist.size(1) != 3) {
        std::ostringstream ss; ss << dist.sizes();
        spdlog::error("[ADC] dist shape unexpected (expected [{},3]): {}", N, ss.str());
        return;
    }

    // If switch_prob was not provided by the model, derive from max(dist):
    // high confidence in any single therapy is treated as a switch signal
    if (!switch_prob.defined()) {
        switch_prob = std::get<0>(dist.max(/*dim=*/1));   // (N,)  values in [0,1]
    }

    if (switch_prob.dim() != 1 || switch_prob.size(0) != N) {
        std::ostringstream ss; ss << switch_prob.sizes();
        spdlog::error("[ADC] switch_prob shape unexpected (expected [{}]): {}", N, ss.str());
        return;
    }

    // ---- Per-unit: smoothing, switch-state update, diagnostic logging ----
    for (int u = 0; u < N; ++u) {
        const float d6 = dist[u][0].item<float>();
        const float d7 = dist[u][1].item<float>();
        const float d8 = dist[u][2].item<float>();
        const float sp = switch_prob[u].item<float>();

        const float dists3[3] = {d6, d7, d8};
        const int raw_cls = static_cast<int>(
            std::max_element(dists3, dists3 + 3) - dists3);
        // raw_cls: 0->th6, 1->th7, 2->th8

        const float tf_now = data.h_current_tf.empty()
                             ? 0.f
                             : static_cast<float>(data.h_current_tf.back()[u]);

        auto& sw  = data.switch_state[u];
        const int sm = smoothed_therapy(sw, raw_cls);
        update_switch_state(sw, t_pb, sm, tf_now);

        if (u < 3) {
            spdlog::info(
                "[ADC INFERENCE] level={} unit={} t={} "
                "dist=[{:.3f},{:.3f},{:.3f}] argmax=th{} sw_prob={:.3f} smoothed=th{}",
                level_id, u, t_pb, d6, d7, d8, raw_cls + 6, sp, sm + 6);
        }
    }

    // ---- Global scheduling decision using unit 0 ----
    {
        const int   u  = 0;
        const float sp = switch_prob[u].item<float>();

        if (sp < sw_threshold) {
            spdlog::debug("[ADC] sw_prob={:.3f} below threshold={:.3f}, no switch scheduled",
                          sp, sw_threshold);
            return;
        }

        const float d6 = dist[u][0].item<float>();
        const float d7 = dist[u][1].item<float>();
        const float d8 = dist[u][2].item<float>();
        const float dists3[3] = {d6, d7, d8};
        const int cls     = static_cast<int>(std::max_element(dists3, dists3 + 3) - dists3);
        const int therapy = cls + 6;   // 0->6, 1->7, 2->8

        if (!therapy_to_strategy_.count(therapy)) {
            spdlog::warn("[ADC] therapy={} not in therapy_to_strategy_, skipping", therapy);
            return;
        }
        const int strategy_id  = therapy_to_strategy_.at(therapy);
        const int forecast_day = now_day + horizon_months * 30;

        if (forecast_day <= now_day) return;

        const auto& strategy_db = Model::get_strategy_db();
        if (strategy_id < 0 || strategy_id >= static_cast<int>(strategy_db.size())) {
            spdlog::error("[ADC] strategy_id={} out of range, not scheduling", strategy_id);
            return;
        }

        auto event = std::make_unique<ChangeTreatmentStrategyEvent>(strategy_id, forecast_day);
        event->set_executable(true);
        Model::get_scheduler()->schedule_population_event(std::move(event));

        spdlog::info("[ADC] Scheduled strategy_id={} at day={} "
                     "(therapy={} sw_prob={:.3f} dist=[{:.3f},{:.3f},{:.3f}])",
                     strategy_id, forecast_day, therapy, sp, d6, d7, d8);

        // Record pending switch and start cooldown
        auto& g            = data.guard[GU];
        g.pending_day      = forecast_day;
        g.pending_strategy = strategy_id;
        g.block_until_day  = now_day + cooldown_days;

        spdlog::info("[ADC] Cooldown until day={}", g.block_until_day);
    }
}

// ============================================================
// ADCAgentData::build_input_batch
//
// Returns N rows each of length W*F, laid out as
//   row[t * F + f]  for t in [0, W), f in [0, F).
//
// 55-feature layout (must match adc_model_v5_5.yml input_features exactly):
//
//   f[ 0]  monthly_number_of_new_infections_by_location      log1p(v/pop)
//   f[ 1]  monthly_number_of_treatment_by_location           log1p(v/pop)
//   f[ 2]  monthly_number_of_clinical_episode_by_location    v/pop
//   f[ 3-13]  monthly_clinical_episode_by_location_age_{0,1,10,2,3,4,5,6,7,8,9}  v/pop
//             (lexicographic age ordering from NPZ)
//   f[14-28]  blood_slide_prevalence_by_location_age_group_{0,1,10,11,12,13,14,2,3,4,5,6,7,8,9}
//             raw [0,1]  (lexicographic age_group ordering from NPZ)
//   f[29-39]  blood_slide_prevalence_by_location_age_{0,1,10,2,3,4,5,6,7,8,9}
//             raw [0,1]  (lexicographic age ordering from NPZ)
//   f[40]  current_TF_by_location                            raw [0,1]
//   f[41]  monthly_number_of_mutation_events_by_location     log1p(v/pop)
//   f[42]  tf_by_therapy_6    raw [0,1]
//   f[43]  tf_by_therapy_7    raw [0,1]
//   f[44]  tf_by_therapy_8    raw [0,1]
//   f[45]  monthly_number_of_TF_by_location                  raw [0,1]  (NOT per-capita)
//   f[46]  ART   f[47] PPQ   f[48] LUM   f[49] AMQ           allele freq [0,1]
//   f[50]  switch_months_since_last   normalised by W
//   f[51]  switch_tf_at_last
//   f[52]  switch_mean_interval       normalised by W
//   f[53]  beta_norm                  constant for this run [0,1]
//   f[54]  t_pos                      position in active period [0,1]
// ============================================================

std::vector<std::vector<float>> AdaptiveCyclingAgent::ADCAgentData::build_input_batch(
    int W, int F,
    float beta_norm,
    int t_pb,
    int burn_in,
    int t_active
) const {
    const int hist_len = history_len();
    if (hist_len < W)
        throw std::runtime_error("[ADC] build_input_batch: not enough history");

    // Window covers deque indices [hist_len-W .. hist_len-1]
    const int deque_start = hist_len - W;

    // Infer n_units from populated deques
    int n_units = 0;
    if (!h_current_tf.empty())  n_units = static_cast<int>(h_current_tf.back().size());
    else if (!h_tf6.empty())    n_units = static_cast<int>(h_tf6.back().size());
    if (n_units <= 0)
        throw std::runtime_error("[ADC] build_input_batch: cannot infer n_units from history");

    const float t_active_f = static_cast<float>(std::max(t_active, 1));
    const float W_f        = static_cast<float>(W);

    // Lexicographic age/age_group sub-index orderings (as stored in NPZ)
    static const int AGE_ORDER[11] = {0, 1, 10, 2, 3, 4, 5, 6, 7, 8, 9};
    static const int AG_ORDER[15]  = {0, 1, 10, 11, 12, 13, 14, 2, 3, 4, 5, 6, 7, 8, 9};

    std::vector<std::vector<float>> batch(n_units, std::vector<float>(W * F, 0.f));

    for (int u = 0; u < n_units; ++u) {
        const SwitchState& sw = switch_state[u];

        for (int k = 0; k < W; ++k) {
            const int di        = deque_start + k;
            const int month_abs = (t_pb - W + 1) + k;
            const int base      = k * F;

            // Bounds-safe scalar accessor
            auto scalar = [&](const std::deque<std::vector<double>>& dq) -> double {
                if (di < 0 || di >= static_cast<int>(dq.size())) return 0.0;
                const auto& v = dq[di];
                return (u < static_cast<int>(v.size())) ? v[u] : 0.0;
            };

            // Bounds-safe 2-D accessor (unit x sub-index)
            auto age2d = [&](const std::deque<std::vector<std::vector<double>>>& dq,
                              int sub) -> double {
                if (di < 0 || di >= static_cast<int>(dq.size())) return 0.0;
                const auto& v = dq[di];
                if (u >= static_cast<int>(v.size())) return 0.0;
                return (sub < static_cast<int>(v[u].size())) ? v[u][sub] : 0.0;
            };

            const double pop = std::max(scalar(h_popsize), 1.0);

            // f[0]  monthly_new_infections   log1p(v/pop)
            batch[u][base +  0] = static_cast<float>(std::log1p(scalar(h_new_infections) / pop));

            // f[1]  monthly_treatment         log1p(v/pop)
            batch[u][base +  1] = static_cast<float>(std::log1p(scalar(h_treatment) / pop));

            // f[2]  monthly_clinical           v/pop
            batch[u][base +  2] = static_cast<float>(scalar(h_clinical) / pop);

            // f[3-13]  monthly_clinical_age  v/pop  (lexicographic: 0,1,10,2..9)
            for (int i = 0; i < 11; ++i)
                batch[u][base + 3 + i] =
                    static_cast<float>(age2d(h_clinical_age, AGE_ORDER[i]) / pop);

            // f[14-28]  bsp_age_group  raw  (lexicographic: 0,1,10,11,12,13,14,2..9)
            for (int i = 0; i < 15; ++i)
                batch[u][base + 14 + i] =
                    static_cast<float>(age2d(h_bsp_age_group, AG_ORDER[i]));

            // f[29-39]  bsp_age  raw  (lexicographic: 0,1,10,2..9)
            for (int i = 0; i < 11; ++i)
                batch[u][base + 29 + i] =
                    static_cast<float>(age2d(h_bsp_age, AGE_ORDER[i]));

            // f[40]  current_TF   raw
            batch[u][base + 40] = static_cast<float>(scalar(h_current_tf));

            // f[41]  monthly_mutation   log1p(v/pop)
            batch[u][base + 41] = static_cast<float>(std::log1p(scalar(h_mutation) / pop));

            // f[42-44]  tf_by_therapy 6/7/8   raw
            batch[u][base + 42] = static_cast<float>(scalar(h_tf6));
            batch[u][base + 43] = static_cast<float>(scalar(h_tf7));
            batch[u][base + 44] = static_cast<float>(scalar(h_tf8));

            // f[45]  monthly_TF   raw  (already a rate, NOT per-capita)
            batch[u][base + 45] = static_cast<float>(scalar(h_monthly_tf));

            // f[46-49]  ART / PPQ / LUM / AMQ   allele freq [0,1]
            batch[u][base + 46] = static_cast<float>(scalar(h_ART));
            batch[u][base + 47] = static_cast<float>(scalar(h_PPQ));
            batch[u][base + 48] = static_cast<float>(scalar(h_LUM));
            batch[u][base + 49] = static_cast<float>(scalar(h_AMQ));

            // f[50]  switch_months_since_last  normalised by W
            batch[u][base + 50] = (sw.last_switch_t >= 0)
                ? static_cast<float>(month_abs - sw.last_switch_t) / W_f
                : 0.f;

            // f[51]  switch_tf_at_last
            batch[u][base + 51] = sw.tf_at_switch;

            // f[52]  switch_mean_interval  normalised by W
            batch[u][base + 52] = sw.mean_interval / W_f;

            // f[53]  beta_norm  constant across all timesteps
            batch[u][base + 53] = beta_norm;

            // f[54]  t_pos  position within active period [0,1]
            batch[u][base + 54] = static_cast<float>(
                std::clamp(static_cast<double>(month_abs - burn_in) / t_active_f,
                           0.0, 1.0));
        }
    }

    return batch;
}

// ============================================================
// Date helpers
// ============================================================

AdaptiveCyclingAgent::YearMonth
AdaptiveCyclingAgent::add_months(int start_year, int start_month, int offset) {
    YearMonth result;
    const int m0 = (start_month - 1) + offset;
    result.year  = start_year + (m0 / 12);
    result.month = (m0 % 12) + 1;
    return result;
}

std::string AdaptiveCyclingAgent::ym_to_string(const YearMonth& ym) {
    std::ostringstream os;
    os << ym.year << "-" << std::setw(2) << std::setfill('0') << ym.month;
    return os.str();
}
