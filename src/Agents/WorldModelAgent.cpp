#include "WorldModelAgent.h"

#include <algorithm>
#include <cmath>
#include <date/date.h>
#include <fstream>
#include <regex>
#include <sstream>

#include "Events/Population/ChangeTreatmentStrategyEvent.h"
#include "Parasites/Genotype.h"
#include "Reporters/SQLiteValidationReporter.h"
#include "Simulation/Model.h"

static constexpr int DAYS_PER_MONTH = 30;

// ── Constructor ──────────────────────────────────────────────────────────────
WorldModelAgent::WorldModelAgent() {
    current_state_.fill(0.f);
    prev_state_.fill(0.f);
    state_mean_.fill(0.f);
    state_std_.fill(1.f);
    delta_mean_.fill(0.f);
    delta_std_.fill(1.f);
    target_mean_.fill(0.f);
    target_std_.fill(1.f);
}

// ── Normalization ────────────────────────────────────────────────────────────
void WorldModelAgent::normalize_state(const float* raw, float* out) const {
    for (int i = 0; i < N_STATE; ++i)
        out[i] = (raw[i] - state_mean_[i]) / state_std_[i];
}

void WorldModelAgent::normalize_delta(const float* raw, float* out) const {
    for (int i = 0; i < N_STATE; ++i)
        out[i] = (raw[i] - delta_mean_[i]) / delta_std_[i];
}

void WorldModelAgent::denormalize_target(const float* norm, float* out) const {
    for (int i = 0; i < N_STATE; ++i)
        out[i] = norm[i] * target_std_[i] + target_mean_[i];
}

// ── Model forward pass ──────────────────────────────────────────────────────
void WorldModelAgent::forward(
    const float* state_n, const float* delta_n,
    const float* action, float* pred_delta_n)
{
    // Build 3 separate input tensors matching the traced signature:
    //   forward(state: Tensor, delta: Tensor, action: Tensor) -> Tensor
    auto t_state  = torch::zeros({1, N_STATE},  torch::kFloat32);
    auto t_delta  = torch::zeros({1, N_STATE},  torch::kFloat32);
    auto t_action = torch::zeros({1, N_ACTION}, torch::kFloat32);

    auto s_acc = t_state.accessor<float, 2>();
    auto d_acc = t_delta.accessor<float, 2>();
    auto a_acc = t_action.accessor<float, 2>();

    for (int i = 0; i < N_STATE; ++i)  s_acc[0][i] = state_n[i];
    for (int i = 0; i < N_STATE; ++i)  d_acc[0][i] = delta_n[i];
    for (int i = 0; i < N_ACTION; ++i) a_acc[0][i] = action[i];

    auto output = model_.forward({t_state, t_delta, t_action}).toTensor();
    auto out_acc = output.accessor<float, 2>();
    for (int i = 0; i < N_STATE; ++i)
        pred_delta_n[i] = out_acc[0][i];
}

// ── Score a predicted trajectory ─────────────────────────────────────────────
float WorldModelAgent::score_trajectory(
    const std::vector<std::array<float, N_STATE>>& trajectory) const
{
    float score = 0.f;
    for (size_t h = 0; h < trajectory.size(); ++h) {
        const auto& s = trajectory[h];
        const float gamma_h = std::pow(discount_, static_cast<float>(h));

        float step_cost = 0.f;
        step_cost += w_tf_  * s[IDX_TF];
        step_cost += w_art_ * s[IDX_ART];
        step_cost += w_ppq_ * s[IDX_PPQ];
        step_cost += w_lum_ * s[IDX_LUM];
        step_cost += w_amq_ * s[IDX_AMQ];

        score += gamma_h * step_cost;
    }
    return score;
}

// ── Plan: roll out 3 therapies, pick best ────────────────────────────────────
int WorldModelAgent::plan(const float* state_raw, const float* delta_raw) {
    const int N_THERAPIES = 3;
    const float actions[N_THERAPIES][3] = {
        {1.f, 0.f, 0.f},  // therapy 6
        {0.f, 1.f, 0.f},  // therapy 7
        {0.f, 0.f, 1.f},  // therapy 8
    };
    const int therapy_ids[N_THERAPIES] = {THERAPY_6, THERAPY_7, THERAPY_8};

    float scores[N_THERAPIES] = {};

    for (int t = 0; t < N_THERAPIES; ++t) {
        // Start from current real state
        std::array<float, N_STATE> s_raw, d_raw;
        std::copy(state_raw, state_raw + N_STATE, s_raw.begin());
        std::copy(delta_raw, delta_raw + N_STATE, d_raw.begin());

        std::vector<std::array<float, N_STATE>> trajectory;

        for (int h = 0; h < planning_horizon_; ++h) {
            // Normalize
            float s_norm[N_STATE], d_norm[N_STATE];
            normalize_state(s_raw.data(), s_norm);
            normalize_delta(d_raw.data(), d_norm);

            // Forward pass
            float pred_delta_norm[N_STATE];
            forward(s_norm, d_norm, actions[t], pred_delta_norm);

            // Denormalize predicted delta
            float pred_delta_raw[N_STATE];
            denormalize_target(pred_delta_norm, pred_delta_raw);

            // Update state: s_{t+1} = s_t + delta
            for (int i = 0; i < N_STATE; ++i) {
                d_raw[i] = pred_delta_raw[i];
                s_raw[i] = s_raw[i] + pred_delta_raw[i];
            }

            trajectory.push_back(s_raw);
        }

        scores[t] = score_trajectory(trajectory);
    }

    // Find best therapy
    int best_idx = 0;
    for (int t = 1; t < N_THERAPIES; ++t)
        if (scores[t] < scores[best_idx]) best_idx = t;

    // Apply hysteresis: only switch if improvement exceeds threshold
    int current_idx = -1;
    for (int t = 0; t < N_THERAPIES; ++t)
        if (therapy_ids[t] == current_therapy_) { current_idx = t; break; }

    if (current_idx >= 0 && best_idx != current_idx) {
        float current_score = scores[current_idx];
        float best_score = scores[best_idx];
        float improvement = (current_score - best_score) /
                            (std::abs(current_score) + 1e-10f);
        if (improvement < switch_threshold_) {
            // Not enough improvement — stay with current therapy
            spdlog::info("[WM] Hysteresis: improvement={:.1f}% < threshold={:.1f}%, staying on th{}",
                         improvement * 100.f, switch_threshold_ * 100.f, current_therapy_);
            return current_therapy_;
        }
    }

    int chosen_therapy = therapy_ids[best_idx];

    spdlog::info("[WM] Plan scores: th6={:.4f} th7={:.4f} th8={:.4f} → best=th{}",
                 scores[0], scores[1], scores[2], chosen_therapy);

    return chosen_therapy;
}

// ── Load model (TorchScript) ─────────────────────────────────────────────────
void WorldModelAgent::load_model() {
    const auto& path = Model::get_config()
        ->get_agent_parameters().get_world_model_agent().get_model_path();
    model_ = torch::jit::load(path);
    model_.eval();
    spdlog::info("[WM] World model loaded: {}", path);
}

// ── Load normalization stats ─────────────────────────────────────────────────
void WorldModelAgent::load_norm_stats() {
    // norm_stats.npz is expected alongside the model
    // We read it as a raw binary npz — or load from a companion YAML.
    // For simplicity, store norm stats in the manifest YAML.
    const auto& manifest = Model::get_config()
        ->get_agent_parameters().get_world_model_agent().get_manifest_path();

    YAML::Node cfg = YAML::LoadFile(manifest);

    auto load_array = [&](const std::string& key, std::array<float, N_STATE>& arr) {
        const auto& node = cfg[key];
        if (!node || !node.IsSequence() || node.size() != N_STATE) {
            throw std::runtime_error(
                "[WM] manifest missing or wrong-sized '" + key +
                "', expected " + std::to_string(N_STATE) + " values");
        }
        for (int i = 0; i < N_STATE; ++i)
            arr[i] = node[i].as<float>();
    };

    load_array("state_mean", state_mean_);
    load_array("state_std", state_std_);
    load_array("delta_mean", delta_mean_);
    load_array("delta_std", delta_std_);
    load_array("target_mean", target_mean_);
    load_array("target_std", target_std_);

    spdlog::info("[WM] Norm stats loaded from manifest");
}

// ── Load manifest ────────────────────────────────────────────────────────────
void WorldModelAgent::load_manifest() {
    const auto& path = Model::get_config()
        ->get_agent_parameters().get_world_model_agent().get_manifest_path();
    YAML::Node cfg = YAML::LoadFile(path);

    // Planning config
    if (cfg["planning_horizon"])   planning_horizon_  = cfg["planning_horizon"].as<int>();
    if (cfg["discount"])           discount_          = cfg["discount"].as<float>();
    if (cfg["switch_threshold"])   switch_threshold_  = cfg["switch_threshold"].as<float>();
    if (cfg["cooldown_months"])    cooldown_months_   = cfg["cooldown_months"].as<int>();
    if (cfg["w_tf"])               w_tf_              = cfg["w_tf"].as<float>();
    if (cfg["w_art"])              w_art_             = cfg["w_art"].as<float>();
    if (cfg["w_ppq"])              w_ppq_             = cfg["w_ppq"].as<float>();
    if (cfg["w_lum"])              w_lum_             = cfg["w_lum"].as<float>();
    if (cfg["w_amq"])              w_amq_             = cfg["w_amq"].as<float>();

    // Allele patterns
    const YAML::Node ap = cfg["allele_patterns"];
    if (!ap || !ap.IsMap())
        throw std::runtime_error("[WM] manifest missing allele_patterns");

    static const std::vector<std::string> REQUIRED = {"ART", "PPQ", "LUM", "AMQ"};
    allele_names_.clear();
    allele_patterns_.clear();
    for (const auto& name : REQUIRED) {
        if (!ap[name] || !ap[name].IsScalar())
            throw std::runtime_error("[WM] allele_patterns missing: " + name);
        allele_names_.push_back(name);
        allele_patterns_.push_back(ap[name].as<std::string>());
    }

    allele_ids_built_ = false;

    spdlog::info("[WM] Manifest loaded: horizon={} discount={:.2f} switch_threshold={:.2f} "
                 "cooldown={}mo", planning_horizon_, discount_, switch_threshold_, cooldown_months_);
    spdlog::info("[WM] Scoring weights: TF={:.2f} ART={:.2f} PPQ={:.2f} LUM={:.2f} AMQ={:.2f}",
                 w_tf_, w_art_, w_ppq_, w_lum_, w_amq_);
}

// ── Initialize ───────────────────────────────────────────────────────────────
void WorldModelAgent::initialize() {
    load_manifest();
    load_norm_stats();
    load_model();

    // Trigger date
    const auto& adc_cfg = Model::get_config()->get_agent_parameters().get_world_model_agent();
    const auto starting = Model::get_config()->get_simulation_timeframe().get_starting_date();
    trigger_day_ = (date::sys_days{adc_cfg.get_trigger_date()} -
                    date::sys_days{starting}).count();

    // Therapy → strategy mapping
    const auto& cycle = adc_cfg.get_strategy_cycle();
    const int therapies[] = {THERAPY_8, THERAPY_7, THERAPY_6};
    for (size_t i = 0; i < 3 && i < cycle.size(); ++i)
        therapy_to_strategy_[therapies[i]] = cycle[i];

    spdlog::info("[WM] therapy→strategy: th8→{} th7→{} th6→{}",
                 therapy_to_strategy_[THERAPY_8],
                 therapy_to_strategy_[THERAPY_7],
                 therapy_to_strategy_[THERAPY_6]);

    spdlog::info("[WM] Initialized. trigger_day={}", trigger_day_);
}

// ── Reset month data ─────────────────────────────────────────────────────────
void WorldModelAgent::reset_month_data() {
    current_state_.fill(0.f);
}

// ── Finalize month: compute allele freqs, fill state, run inference ──────────
void WorldModelAgent::finalize_month_features(
    int numGenotypes,
    const std::vector<SQLiteValidationReporter::MonthlyGenomeData>& genome_data,
    int level_id)
{
    // ── Lazy-build allele genotype ID sets ────────────────────────────────
    if (!allele_ids_built_ || numGenotypes > allele_ids_n_geno_) {
        auto* gdb = Model::get_genotype_db();
        const int n_geno = static_cast<int>(gdb->size());
        const int n_alleles = static_cast<int>(allele_patterns_.size());

        std::vector<std::regex> compiled(n_alleles);
        for (int a = 0; a < n_alleles; ++a)
            compiled[a] = std::regex(allele_patterns_[a]);

        allele_genotype_ids_.assign(n_alleles, std::unordered_set<int>{});

        for (int g = 0; g < n_geno; ++g) {
            const std::string name =
                Genotype::convert_pf_genotype_str_to_string(gdb->at(g)->pf_genotype_str);
            for (int a = 0; a < n_alleles; ++a)
                if (std::regex_search(name, compiled[a]))
                    allele_genotype_ids_[a].insert(g);
        }

        for (int a = 0; a < n_alleles; ++a)
            spdlog::info("[WM] Allele {} matched {} / {} genotypes",
                         allele_names_[a], allele_genotype_ids_[a].size(), n_geno);

        allele_ids_built_ = true;
        allele_ids_n_geno_ = n_geno;
    }

    // ── Compute allele frequencies for unit 0 ─────────────────────────────
    const int u = 0;
    const int n_alleles = static_cast<int>(allele_genotype_ids_.size());
    double tot = 0.0;
    std::vector<double> sums(n_alleles, 0.0);

    for (int g = 0; g < numGenotypes; ++g) {
        const double w = genome_data[level_id].weighted_occurrences[u][g];
        if (w <= 0.0) continue;
        tot += w;
        for (int a = 0; a < n_alleles; ++a)
            if (allele_genotype_ids_[a].count(g)) sums[a] += w;
    }
    const double denom = tot > 0.0 ? tot : 1.0;

    // ── Fill state vector (24 features in training order) ───────────────
    auto* mdc = Model::get_mdc();
    const int loc = 0;

    current_state_[IDX_TF]         = static_cast<float>(mdc->current_tf_by_location()[loc]);
    current_state_[IDX_TF_COUNT]   = static_cast<float>(mdc->monthly_number_of_tf_by_location()[loc]);
    current_state_[IDX_CLINICAL]   = static_cast<float>(mdc->monthly_number_of_clinical_episode_by_location()[loc]);

    // clinical_episode_by_location_age: indices 3..13 → ages 0,1,10,2,3,4,5,6,7,8,9
    // (NPZ lexicographic sort: 0,1,10,2,3,4,5,6,7,8,9)
    static const int AGE_ORDER[] = {0, 1, 10, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int k = 0; k < 11; ++k) {
        current_state_[3 + k] = static_cast<float>(
            mdc->monthly_number_of_clinical_episode_by_location_age()[loc][AGE_ORDER[k]]);
    }

    current_state_[IDX_MUTATION]   = static_cast<float>(mdc->monthly_number_of_mutation_events_by_location()[loc]);
    current_state_[IDX_INFECTIONS] = static_cast<float>(mdc->monthly_number_of_new_infections_by_location()[loc]);
    current_state_[IDX_TREATMENT]  = static_cast<float>(mdc->monthly_number_of_treatment_by_location()[loc]);
    current_state_[IDX_TF6]        = static_cast<float>(mdc->current_tf_by_therapy()[6]);
    current_state_[IDX_TF7]        = static_cast<float>(mdc->current_tf_by_therapy()[7]);
    current_state_[IDX_TF8]        = static_cast<float>(mdc->current_tf_by_therapy()[8]);
    current_state_[IDX_ART]        = static_cast<float>(sums[0] / denom);
    current_state_[IDX_PPQ]        = static_cast<float>(sums[1] / denom);
    current_state_[IDX_LUM]        = static_cast<float>(sums[2] / denom);
    current_state_[IDX_AMQ]        = static_cast<float>(sums[3] / denom);

    // Clamp NaN/Inf
    for (int i = 0; i < N_STATE; ++i)
        if (!std::isfinite(current_state_[i])) current_state_[i] = 0.f;

    // ── Run inference if ready ────────────────────────────────────────────
    run_inference();

    // Save for next month's delta
    std::copy(current_state_.begin(), current_state_.end(), prev_state_.begin());
    has_prev_state_ = true;
}

// ── Run inference ────────────────────────────────────────────────────────────
void WorldModelAgent::run_inference() {
    const int now = Model::get_scheduler()->current_time();

    // Not ready yet
    if (now < trigger_day_ || !has_prev_state_) return;

    // Cooldown
    if (cooldown_remaining_ > 0) {
        cooldown_remaining_--;
        return;
    }

    // Compute delta
    std::array<float, N_STATE> delta_raw;
    for (int i = 0; i < N_STATE; ++i)
        delta_raw[i] = current_state_[i] - prev_state_[i];

    // Plan
    int best_therapy = plan(current_state_.data(), delta_raw.data());

    // If same as current, do nothing
    if (best_therapy == current_therapy_) return;

    // Switch therapy
    auto it = therapy_to_strategy_.find(best_therapy);
    if (it == therapy_to_strategy_.end()) {
        spdlog::warn("[WM] therapy {} not in therapy_to_strategy map", best_therapy);
        return;
    }

    const int strategy_id = it->second;

    // Schedule the change immediately (next month)
    const int switch_day = now + DAYS_PER_MONTH;
    auto event = std::make_unique<ChangeTreatmentStrategyEvent>(strategy_id, switch_day);
    event->set_executable(true);
    Model::get_scheduler()->schedule_population_event(std::move(event));

    spdlog::info("[WM] SWITCH: th{} → th{} (strategy={}) at day={} "
                 "(TF={:.3f} ART={:.3f} PPQ={:.4f} LUM={:.4f} AMQ={:.3f})",
                 current_therapy_, best_therapy, strategy_id, switch_day,
                 current_state_[IDX_TF],
                 current_state_[IDX_ART], current_state_[IDX_PPQ],
                 current_state_[IDX_LUM], current_state_[IDX_AMQ]);

    current_therapy_ = best_therapy;
    cooldown_remaining_ = cooldown_months_;
}
