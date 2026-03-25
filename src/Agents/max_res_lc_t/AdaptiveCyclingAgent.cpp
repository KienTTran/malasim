#include "../max_res_lc_t/AdaptiveCyclingAgent.h"

#include <algorithm>
#include <cmath>
#include <date/date.h>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "Events/Population/ChangeTreatmentStrategyEvent.h"
#include "Parasites/Genotype.h"
#include "Reporters/SQLiteMonthlyReporter.h"
#include "Reporters/SQLiteValidationReporter.h"
#include "Simulation/Model.h"

// ── Simulation constants (must match training script) ─────────────────────────
static constexpr int   BURN_IN         = 120;
static constexpr int   T_ACTIVE        = 241;
static constexpr float BETA_MIN        = 0.089f;
static constexpr float BETA_MAX        = 0.630f;
static constexpr int   SMOOTH_HW       = 6;
static constexpr int   COOLDOWN_MONTHS = 24;
static constexpr int   DAYS_PER_MONTH  = 30;
static constexpr int   THERAPY_6       = 6;
static constexpr int   THERAPY_7       = 7;
static constexpr int   THERAPY_8       = 8;

// ── Allele regex patterns are loaded from the manifest YAML (allele_patterns section)
// into allele_names_ and allele_patterns_ in load_manifest().
// The lazy build in finalize_month_all_features() compiles them into std::regex.
// No hardcoded patterns here — edit the YAML to change matching behaviour.

// ── Age sub-indices present in NPZ (verified from checkpoint) ─────────────────
// blood_slide_prevalence_by_location_age and monthly_clinical_age:
//   NPZ ordering (feature_cols indices): 0,1,10,2,3,4,5,6,7,8,9
//   i.e. MDC age indices: {0,1,10,2,3,4,5,6,7,8,9} in that order
// blood_slide_prevalence_by_location_age_group:
//   NPZ ordering: 0,1,10,11,12,13,14,2,3,4,5,6,7,8,9
//   i.e. MDC age_group indices: {0,1,10,11,12,13,14,2,3,4,5,6,7,8,9}
//
// IMPORTANT: the deque stores these in the NPZ ordering, NOT 0..10 sequential.
// Each [unit][k] slot corresponds to the k-th age in the ordering above.

static constexpr int N_AGE_SINGLE  = 11;   // age indices in bsp_age / clinical_age
static constexpr int N_AGE_GROUP   = 15;   // age-group indices in bsp_age_group

// MDC age index for each slot k in bsp_age and clinical_age
static const int AGE_SINGLE_IDX[N_AGE_SINGLE]  = {0,1,10,2,3,4,5,6,7,8,9};
// MDC age-group index for each slot k in bsp_age_group
static const int AGE_GROUP_IDX[N_AGE_GROUP]    = {0,1,10,11,12,13,14,2,3,4,5,6,7,8,9};


// ── Constructor ───────────────────────────────────────────────────────────────
AdaptiveCyclingAgent::AdaptiveCyclingAgent() = default;

// ── YAML helpers ──────────────────────────────────────────────────────────────
int AdaptiveCyclingAgent::as_int_or_throw(const YAML::Node& n, const char* path) {
    if (!n || !n.IsScalar())
        throw std::runtime_error(std::string("Missing/non-scalar: ") + path);
    try { return n.as<int>(); }
    catch (...) { throw std::runtime_error(std::string("Bad int at ") + path); }
}

std::string AdaptiveCyclingAgent::as_str_or_throw(const YAML::Node& n, const char* path) {
    if (!n || !n.IsScalar())
        throw std::runtime_error(std::string("Missing/non-scalar: ") + path);
    return n.as<std::string>();
}

// ── Load model ────────────────────────────────────────────────────────────────
void AdaptiveCyclingAgent::load_model() {
    const auto& path = Model::get_config()
        ->get_agent_parameters().get_adc_agent().get_model_path();
    agent_raw_model_ = ModelLoader::load_model(path);
    agent_raw_model_.eval();
    agent_model_ = { agent_meta_, agent_raw_model_, torch::kCPU };
    spdlog::info("[ADC] v5.5 model loaded: {}", path);
}

// ── Load manifest ─────────────────────────────────────────────────────────────
void AdaptiveCyclingAgent::load_manifest() {
    const auto& path = Model::get_config()
        ->get_agent_parameters().get_adc_agent().get_manifest_path();
    agent_raw_manifest_ = ModelLoader::load_manifest(path);

    // Accept window.W (v5.5 manifest) or window.L (old manifest)
    const YAML::Node win = agent_raw_manifest_["window"];
    if (win["W"] && win["W"].IsScalar())
        agent_meta_.W = win["W"].as<int>();
    else if (win["L"] && win["L"].IsScalar())
        agent_meta_.W = win["L"].as<int>();
    else
        throw std::runtime_error("manifest window: neither 'W' nor 'L' found");

    const YAML::Node feats = agent_raw_manifest_["input_features"];
    if (!feats || !feats.IsSequence())
        throw std::runtime_error("input_features missing in manifest");
    agent_meta_.input_features.clear();
    for (size_t i = 0; i < feats.size(); ++i)
        agent_meta_.input_features.push_back(
            as_str_or_throw(feats[i], "input_features[i]"));
    agent_meta_.F = static_cast<int>(agent_meta_.input_features.size());

    // v5.5 tf_excess_t model expects exactly 56 input features
    // (50 YAML + 3 switch-timing + 2 context + 1 tf_excess)
    if (agent_meta_.W <= 0)
        throw std::runtime_error("[ADC] manifest window W must be > 0");
    if (agent_meta_.F != 56)
        throw std::runtime_error(
            "[ADC] manifest input_features must list all 56 features "
            "(50 YAML + 3 switch-timing + beta_norm + t_pos + tf_excess), got F=" +
            std::to_string(agent_meta_.F) + ". Are you loading the new adc_model_v5_5.yml?");

    // Load allele_patterns section — order must be ART, PPQ, LUM, AMQ
    // to match model feature slots f[46..49].
    const YAML::Node ap = agent_raw_manifest_["allele_patterns"];
    if (!ap || !ap.IsMap())
        throw std::runtime_error(
            "[ADC] manifest missing 'allele_patterns' map. "
            "Expected keys: ART, PPQ, LUM, AMQ in that order.");

    // Required key order — must match training feature order f[46..49]
    static const std::vector<std::string> REQUIRED_ALLELES = {"ART","PPQ","LUM","AMQ"};
    allele_names_.clear();
    allele_patterns_.clear();
    for (const auto& name : REQUIRED_ALLELES) {
        if (!ap[name] || !ap[name].IsScalar())
            throw std::runtime_error(
                "[ADC] manifest allele_patterns missing key: " + name);
        allele_names_.push_back(name);
        allele_patterns_.push_back(ap[name].as<std::string>());
    }

    // Reset lazy build so next call to finalize_month_all_features()
    // will recompile regex from the newly loaded patterns.
    allele_genotype_ids_.clear();
    allele_genotype_ids_built_  = false;
    allele_genotype_ids_n_geno_ = 0;

    spdlog::info("[ADC] Manifest: W={} F={}", agent_meta_.W, agent_meta_.F);
    for (size_t a = 0; a < allele_names_.size(); ++a)
        spdlog::info("[ADC]   allele_patterns[{}] {}=\'{}\'",
                     a, allele_names_[a], allele_patterns_[a]);
}

// ── Initialize ────────────────────────────────────────────────────────────────
void AdaptiveCyclingAgent::initialize() {
    load_manifest();
    load_model();

    // Beta normalisation from config
    const float beta_raw = static_cast<float>(Model::get_config()->location_db()[0].beta);
    beta_norm_ = std::clamp((beta_raw - BETA_MIN) / (BETA_MAX - BETA_MIN), 0.f, 1.f);
    spdlog::info("[ADC] beta={:.4f}  beta_norm={:.4f}", beta_raw, beta_norm_);

    // Trigger date → trigger_day_ and trigger_month_
    const auto& adc_cfg   = Model::get_config()->get_agent_parameters().get_adc_agent();
    trigger_value_         = adc_cfg.get_trigger_value();
    const auto starting    = Model::get_config()->get_simulation_timeframe().get_starting_date();
    trigger_day_           = (date::sys_days{adc_cfg.get_trigger_date()} -
                              date::sys_days{starting}).count();
    trigger_month_         = static_cast<int>(trigger_day_) / DAYS_PER_MONTH;
    sim_start_year_        = static_cast<int>(starting.year());
    sim_start_month_       = 0;

    if (trigger_day_ < 0)
        throw std::runtime_error("[ADC] trigger_date before simulation start");
    if (trigger_month_ < agent_meta_.W)
        throw std::runtime_error("[ADC] trigger_date too early: need >= W months of history");
    spdlog::info("[ADC] trigger_value={:.3f} trigger_day={} trigger_month={}",
                 trigger_value_, trigger_day_, trigger_month_);

    // therapy → strategy mapping from config strategy_cycle
    // strategy_cycle in config YAML: list of strategy_ids, one per therapy in order [th8, th7, th6]
    // Example: strategy_cycle: [2, 1, 3]  →  th8→strategy2, th7→strategy1, th6→strategy3
    // If fewer than 3 entries, remaining therapies reuse the last entry.
    const auto& cycle = adc_cfg.get_strategy_cycle();
    spdlog::info("[ADC] strategy_cycle from config: {} entries", cycle.size());
    for (size_t i = 0; i < cycle.size(); ++i)
        spdlog::info("[ADC]   cycle[{}] = strategy_id {}", i, cycle[i]);

    if (cycle.empty()) {
        spdlog::warn("[ADC] strategy_cycle is empty — strategy switching disabled. "
                     "Add 'strategy_cycle: [id8, id7, id6]' to your config adc_agent section.");
    } else {
        // Map therapy 8, 7, 6 → strategy ids using cycle in order
        // If cycle has fewer than 3 entries, repeat the last one
        const std::vector<int> therapy_order = {THERAPY_8, THERAPY_7, THERAPY_6};
        for (size_t i = 0; i < therapy_order.size(); ++i) {
            const int strat = cycle[std::min(i, cycle.size() - 1)];
            therapy_to_strategy_[therapy_order[i]] = strat;
        }
        spdlog::info("[ADC] therapy→strategy map: th8→{} th7→{} th6→{}",
                     therapy_to_strategy_[THERAPY_8],
                     therapy_to_strategy_[THERAPY_7],
                     therapy_to_strategy_[THERAPY_6]);
    }

    // Resize per-level data structures
    const auto* admin  = Model::get_spatial_data()->get_admin_level_manager();
    const int n_levels = static_cast<int>(admin->get_level_names().size());
    adc_agent_data_by_level.resize(n_levels + 1);

    for (int lv = 0; lv < n_levels + 1; ++lv) {
        const bool is_cell = (lv == n_levels);
        if (is_cell && n_levels > 0) continue;
        const int vsz = is_cell
            ? Model::get_config()->number_of_locations()
            : [&]() -> int {
                const auto* b = admin->get_boundary(admin->get_level_names()[lv]);
                return b ? b->max_unit_id + 1 : 0;
              }();
        if (vsz <= 0) continue;
        auto& d = adc_agent_data_by_level[lv];
        d.set_history_cap(agent_meta_.W);
        d.reset_month(vsz);
        d.ensure_state_size(vsz);
    }

    spdlog::info("[ADC] Initialized. W={} F={}", agent_meta_.W, agent_meta_.F);
    // Allele genotype ID sets are built lazily on first call to
    // finalize_month_all_features(), once the full genotype DB is populated.
    allele_genotype_ids_built_ = false;
}

// ── Switch-timing helpers ─────────────────────────────────────────────────────
int AdaptiveCyclingAgent::smoothed_therapy(SwitchState& sw, int raw) {
    sw.raw_buf.push_back(raw);
    // Trim to the window we actually vote over
    const int cap = 2 * SMOOTH_HW + 1;
    if (static_cast<int>(sw.raw_buf.size()) > cap)
        sw.raw_buf.erase(sw.raw_buf.begin());
    int cnt[3] = {0, 0, 0};
    for (int v : sw.raw_buf) cnt[v]++;
    return static_cast<int>(std::max_element(cnt, cnt + 3) - cnt);
}

void AdaptiveCyclingAgent::update_switch_state(
    SwitchState& sw, int t_pb, int sm, float tf)
{
    if (sw.prev_smoothed >= 0 && sm != sw.prev_smoothed) {
        if (sw.last_switch_t >= 0) {
            const int iv = t_pb - sw.last_switch_t;
            sw.n_intervals++;
            sw.mean_interval += (iv - sw.mean_interval) / sw.n_intervals;
        }
        sw.last_switch_t = t_pb;
        sw.tf_at_switch  = tf;
    }
    sw.prev_smoothed = sm;
}

std::array<float,3> AdaptiveCyclingAgent::switch_feats(int t_pb, const SwitchState& sw) {
    return {
        sw.last_switch_t >= 0
            ? std::log1p(static_cast<float>(t_pb - sw.last_switch_t))
            : std::log1p(static_cast<float>(t_pb)),
        sw.tf_at_switch,
        sw.n_intervals >= 1 ? std::log1p(sw.mean_interval) : 0.f
    };
}

// ── reset_adc_data ────────────────────────────────────────────────────────────
void AdaptiveCyclingAgent::reset_adc_data(int level_id, int vector_size) {
    auto& d = adc_agent_data_by_level[level_id];
    d.ensure_state_size(vector_size);
    d.reset_month(vector_size);
}

// ── finalize_month_all_features ───────────────────────────────────────────────
void AdaptiveCyclingAgent::finalize_month_all_features(
    int level_id,
    int numGenotypes,
    const std::vector<SQLiteValidationReporter::MonthlyGenomeData>& genome_data)
{
    auto& d   = adc_agent_data_by_level[level_id];
    const int n   = static_cast<int>(d.freq_ART.size());

    // ── Lazy-build allele genotype ID sets ────────────────────────────────────
    // Done here (not in initialize()) because the full genotype DB — including
    // mutants introduced via population events — is only complete once the
    // simulation is running. Rebuilds if new genotypes have been added since
    // last call (numGenotypes > last known count).
    if (!allele_genotype_ids_built_ || numGenotypes > allele_genotype_ids_n_geno_) {
        auto* gdb = Model::get_genotype_db();
        const int n_geno = static_cast<int>(gdb->size());

        const int n_alleles = static_cast<int>(allele_patterns_.size());
        if (n_alleles == 0)
            throw std::runtime_error(
                "[ADC] allele_patterns_ is empty — was load_manifest() called?");

        // Compile regex for each allele pattern loaded from manifest
        std::vector<std::regex> compiled(n_alleles);
        for (int a = 0; a < n_alleles; ++a)
            compiled[a] = std::regex(allele_patterns_[a]);

        allele_genotype_ids_.assign(n_alleles, std::unordered_set<int>{});
        std::vector<int> matched(n_alleles, 0);

        for (int g = 0; g < n_geno; ++g) {
            const std::string name =
                Genotype::convert_pf_genotype_str_to_string(gdb->at(g)->pf_genotype_str);
            for (int a = 0; a < n_alleles; ++a) {
                if (std::regex_search(name, compiled[a])) {
                    allele_genotype_ids_[a].insert(g);
                    matched[a]++;
                }
            }
        }

        for (int a = 0; a < n_alleles; ++a) {
            spdlog::info("[ADC] Allele {} pattern='{}' matched {} / {} genotypes",
                         allele_names_[a], allele_patterns_[a], matched[a], n_geno);
            if (matched[a] == 0)
                spdlog::warn("[ADC] Allele {} matched 0 genotypes — "
                             "check pattern against genotype names in your config",
                             allele_names_[a]);
        }

        allele_genotype_ids_built_  = true;
        allele_genotype_ids_n_geno_ = n_geno;
    }

    // Compute allele frequencies using pre-computed genotype ID sets.
    // allele_genotype_ids_[a] corresponds to allele_names_[a] in manifest order.
    // Feature slots: [0]=ART f[46], [1]=PPQ f[47], [2]=LUM f[48], [3]=AMQ f[49].
    const int n_alleles = static_cast<int>(allele_genotype_ids_.size());
    for (int u = 0; u < n; ++u) {
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
        // Store in fixed members — order guaranteed by REQUIRED_ALLELES in load_manifest()
        if (n_alleles > 0) d.freq_ART[u] = sums[0] / denom;
        if (n_alleles > 1) d.freq_PPQ[u] = sums[1] / denom;
        if (n_alleles > 2) d.freq_LUM[u] = sums[2] / denom;
        if (n_alleles > 3) d.freq_AMQ[u] = sums[3] / denom;
    }

    // tf6/7/8 are global rates — read once here, just before pushing history.
    // This avoids the monthly_report_site_data / monthly_report_genome_data
    // call-order dependency.
    for (int u = 0; u < n; ++u) {
        d.tf6[u] = Model::get_mdc()->current_tf_by_therapy()[6];
        d.tf7[u] = Model::get_mdc()->current_tf_by_therapy()[7];
        d.tf8[u] = Model::get_mdc()->current_tf_by_therapy()[8];
    }

    d.push_month_all();

    const int now_day = Model::get_scheduler()->current_time();
    if (d.history_len() >= agent_meta_.W && now_day >= trigger_day_)
        inference_from_adc_data(level_id);
}

// ── build_input_batch ─────────────────────────────────────────────────────────
std::vector<std::vector<float>>
AdaptiveCyclingAgent::ADCAgentData::build_input_batch(
    int W, int F,
    float beta_norm,
    int month_abs, int burn_in, int t_active) const
{
    const int T       = history_len();
    const int t_start = T - W;
    if (T < W) throw std::runtime_error("build_input_batch: not enough history");

    const int n_units = static_cast<int>(h_current_tf.back().size());
    if (n_units == 0) throw std::runtime_error("build_input_batch: empty history");

    std::vector<std::vector<float>> batch(n_units, std::vector<float>(W * F, 0.f));

    for (int u = 0; u < n_units; ++u) {
        const SwitchState& sw = switch_state[u];

        for (int w = 0; w < W; ++w) {
            const int t     = t_start + w;
            const int t_pb  = (month_abs - burn_in) - (W - 1 - w);
            const float pop = static_cast<float>(std::max(h_popsize[t][u], 1.0));
            const float t_pos = std::clamp(
                static_cast<float>(t_pb) / static_cast<float>(std::max(t_active-1, 1)),
                0.f, 1.f);
            const auto sf = switch_feats(t_pb, sw);

            float* row = batch[u].data() + w * F;

            // ── 50 YAML features in exact training order ──────────────────
            // f[0]  monthly_number_of_new_infections_by_location
            row[ 0] = std::log1p(static_cast<float>(h_new_infections[t][u]) / pop);

            // f[1]  monthly_number_of_treatment_by_location
            row[ 1] = std::log1p(static_cast<float>(h_treatment[t][u]) / pop);

            // f[2]  monthly_number_of_clinical_episode_by_location
            row[ 2] = static_cast<float>(h_clinical[t][u]) / pop;

            // f[3..13]  monthly_clinical_episode_by_location_age_{0,1,10,2,3,4,5,6,7,8,9}
            for (int k = 0; k < N_AGE_SINGLE; ++k)
                row[3 + k] = static_cast<float>(h_clinical_age[t][u][k]) / pop;

            // f[14..28]  blood_slide_prevalence_by_location_age_group — divide by pop
            for (int k = 0; k < N_AGE_GROUP; ++k)
                row[14 + k] = static_cast<float>(h_bsp_age_group[t][u][k]) / pop;

            // f[29..39]  blood_slide_prevalence_by_location_age — divide by pop
            for (int k = 0; k < N_AGE_SINGLE; ++k)
                row[29 + k] = static_cast<float>(h_bsp_age[t][u][k]) / pop;

            // f[40]  current_TF_by_location
            row[40] = static_cast<float>(h_current_tf[t][u]);

            // f[41]  monthly_number_of_mutation_events_by_location
            row[41] = std::log1p(static_cast<float>(h_mutation[t][u]) / pop);

            // f[42..44]  tf_by_therapy_6/7/8
            row[42] = static_cast<float>(h_tf6[t][u]);
            row[43] = static_cast<float>(h_tf7[t][u]);
            row[44] = static_cast<float>(h_tf8[t][u]);

            // f[45]  monthly_number_of_TF_by_location
            row[45] = static_cast<float>(h_monthly_tf[t][u]) / pop;

            // f[46..49]  ART, PPQ, LUM, AMQ
            row[46] = static_cast<float>(h_ART[t][u]);
            row[47] = static_cast<float>(h_PPQ[t][u]);
            row[48] = static_cast<float>(h_LUM[t][u]);
            row[49] = static_cast<float>(h_AMQ[t][u]);

            // ── Switch-timing + context ───────────────────────────────────
            row[50] = sf[0];   // log1p(months since last switch)
            row[51] = sf[1];   // TF at last switch
            row[52] = sf[2];   // log1p(mean switch interval)
            row[53] = beta_norm;
            row[54] = t_pos;

            // f[55]  tf_excess = ReLU(max(tf6, tf7, tf8) - 0.10)
            // Replicates the ADC trigger signal the manual rule uses.
            // Trained as: np.clip(max_therapy_TF - 0.10, 0, None)
            {
                const float tf_max = std::max({static_cast<float>(h_tf6[t][u]),
                                               static_cast<float>(h_tf7[t][u]),
                                               static_cast<float>(h_tf8[t][u])});
                row[55] = std::max(tf_max - 0.10f, 0.f);
            }

            // Clamp all values to finite — guards against zero popsize or
            // uninitialised MDC fields producing NaN in the model input.
            for (int f = 0; f < F; ++f)
                if (!std::isfinite(row[f])) row[f] = 0.f;
        }
    }
    return batch;
}

// ── inference_from_adc_data ───────────────────────────────────────────────────
void AdaptiveCyclingAgent::inference_from_adc_data(int level_id) {
    auto& d        = adc_agent_data_by_level[level_id];
    const int W    = agent_meta_.W;
    const int F    = agent_meta_.F;
    const int now  = Model::get_scheduler()->current_time();
    const int month_abs = now / DAYS_PER_MONTH;

    d.ensure_state_size(static_cast<int>(d.h_current_tf.back().size()));

    // Global guard (unit 0 — strategy is simulation-wide)
    auto& g = d.guard[0];
    if (g.pending_day != -1 && now >= g.pending_day) {
        g.pending_day      = -1;
        g.pending_strategy = -1;
        g.block_until_day  = now + COOLDOWN_MONTHS * DAYS_PER_MONTH;
    }
    if ((g.block_until_day != -1 && now < g.block_until_day) ||
        (g.pending_day     != -1 && g.pending_day > now))
        return;

    // Build input tensor
    std::vector<std::vector<float>> batch;
    try {
        batch = d.build_input_batch(W, F, beta_norm_, month_abs, BURN_IN, T_ACTIVE);
    } catch (const std::exception& e) {
        spdlog::warn("[ADC] build_input_batch: {}", e.what());
        return;
    }
    const int N = static_cast<int>(batch.size());
    if (N == 0) return;

    // Check for NaN/Inf in batch before building tensor — log and abort if found
    bool has_nan = false;
    for (int b = 0; b < N && !has_nan; ++b)
        for (float v : batch[b])
            if (!std::isfinite(v)) { has_nan = true; break; }
    if (has_nan) {
        spdlog::warn("[ADC] month={} input batch contains NaN/Inf — skipping inference. "
                     "Check that popsize and MDC feature vectors are populated.",
                     month_abs);
        return;
    }

    auto x = torch::zeros({N, W, F}, torch::kFloat32);
    for (int b = 0; b < N; ++b)
        for (int w = 0; w < W; ++w)
            for (int f = 0; f < F; ++f)
                x[b][w][f] = batch[b][w * F + f];

    // Run model
    torch::jit::IValue result;
    try {
        result = agent_model_.module.get_method("predict_cpp")(
            std::vector<torch::jit::IValue>{x});
    } catch (const std::exception& e) {
        spdlog::error("[ADC] predict_cpp failed: {}", e.what());
        return;
    }

    auto tup     = result.toTuple();
    auto dist_t  = tup->elements()[0].toTensor();   // (N, 3) [d6,d7,d8]
    auto sw_t    = tup->elements()[1].toTensor();   // (N,)

    // Use unit 0 for global decision
    const float sw_prob = sw_t[0].item<float>();
    const float d6 = dist_t[0][0].item<float>();
    const float d7 = dist_t[0][1].item<float>();
    const float d8 = dist_t[0][2].item<float>();

    spdlog::info("[ADC] month={} sw_prob={:.3f} dist=[d6={:.3f} d7={:.3f} d8={:.3f}]",
                 month_abs, sw_prob, d6, d7, d8);

    if (sw_prob < static_cast<float>(trigger_value_)) return;

    // Require a clearly dominant therapy before switching
    static constexpr float DOMINANCE_THRESHOLD = 0.85f;   // try 0.70 first
    // static constexpr float DOMINANCE_THRESHOLD = 0.80f; // stricter option

    const float max_dist = std::max({d6, d7, d8});
    if (max_dist < DOMINANCE_THRESHOLD) {
        spdlog::info("[ADC] No dominant therapy (max_dist={:.3f} < {:.3f}) — skip switch",
                     max_dist, DOMINANCE_THRESHOLD);
        return;
    }

    // Dominant therapy
    int therapy_id;
    if      (d8 >= d7 && d8 >= d6) therapy_id = THERAPY_8;
    else if (d7 >= d6)              therapy_id = THERAPY_7;
    else                            therapy_id = THERAPY_6;

    auto it = therapy_to_strategy_.find(therapy_id);
    if (it == therapy_to_strategy_.end()) {
        spdlog::warn("[ADC] therapy {} not in therapy_to_strategy map", therapy_id);
        return;
    }
    const int strategy_id  = it->second;
    const int forecast_day = now + W * DAYS_PER_MONTH;  // now + 24 months

    auto event = std::make_unique<ChangeTreatmentStrategyEvent>(strategy_id, forecast_day);
    event->set_executable(true);
    Model::get_scheduler()->schedule_population_event(std::move(event));

    spdlog::info("[ADC] Scheduled strategy={} therapy={} at day={} (+{}mo)",
                 strategy_id, therapy_id, forecast_day, W);

    g.pending_day      = forecast_day;
    g.pending_strategy = strategy_id;
    // g.block_until_day  = now + COOLDOWN_MONTHS * DAYS_PER_MONTH;
}

// ── YearMonth helpers ─────────────────────────────────────────────────────────
AdaptiveCyclingAgent::YearMonth
AdaptiveCyclingAgent::add_months(int y, int m, int offset) {
    int m0 = (m - 1) + offset;
    return {y + m0 / 12, m0 % 12 + 1};
}
std::string AdaptiveCyclingAgent::ym_to_string(const YearMonth& ym) {
    std::ostringstream o;
    o << ym.year << "-" << std::setw(2) << std::setfill('0') << ym.month;
    return o.str();
}
