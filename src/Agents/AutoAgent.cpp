#include "AutoAgent.h"

#include <algorithm>
#include <cmath>
#include <date/date.h>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "Events/Population/ChangeTreatmentStrategyEvent.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Simulation/Model.h"

// ── Simulation constants (must match training script / summary.txt) ───────────
static constexpr int   AA_BURN_IN      = 120;   ///< months discarded before windowing
static constexpr int   AA_DAYS_PER_MONTH = 30;

// NPZ ordering of age sub-indices — replicated from summary.txt section 4b.
// Must stay in sync with the training feature list.
static constexpr int N_AGE_SINGLE = 11;
static constexpr int N_AGE_GROUP  = 15;
static constexpr int N_AGE_POP    = 11;

/// MDC age index for slot k in clinical_age, bsp_age, popsize_age
static const int AGE_SINGLE_IDX[N_AGE_SINGLE] = {0,1,10,2,3,4,5,6,7,8,9};

/// MDC age-group index for slot k in bsp_age_group, clinical_ag
static const int AGE_GROUP_IDX[N_AGE_GROUP]   = {0,1,10,11,12,13,14,2,3,4,5,6,7,8,9};

// Feature index ranges inside the 92-column YAML feature block (per timestep):
//   [0]      monthly_number_of_new_infections_by_location
//   [1]      monthly_number_of_treatment_by_location
//   [2..13]  monthly_number_of_clinical_episode_by_location  +  _age_0..10  (12 values)
//   [14..23] multiple_of_infection_0..9
//   [24..38] blood_slide_prevalence_by_location_age_group_0..14  (15 values)
//   [39..49] blood_slide_prevalence_by_location_age_0..10         (11 values, slot order: 0,1,10,2..9)
//   [50]     current_TF_by_location
//   [51]     monthly_number_of_mutation_events_by_location
//   [52]     total_number_of_bites_by_location
//   [53]     total_number_of_bites_by_location_year
//   [54]     today_number_of_treatments_by_location
//   [55]     current_number_of_mutation_events_in_this_year
//   [56]     popsize_by_location_hoststate
//   [57..67] popsize_by_location_age_0..10
//   [68..82] number_of_clinical_by_location_age_group_0..14
//   [83]     total_immune_by_location
//   [84..86] tf_by_therapy_6/7/8
//   [87]     monthly_number_of_TF_by_location
//   [88..91] ART, PPQ, LUM, AMQ  (allele frequencies)
//   Total: 92 features

// ── Constructor ───────────────────────────────────────────────────────────────
AutoAgent::AutoAgent() = default;

// ── YAML helpers ──────────────────────────────────────────────────────────────
std::string AutoAgent::as_str_or_throw(const YAML::Node& n, const char* path) {
    if (!n || !n.IsScalar())
        throw std::runtime_error(std::string("[AutoAgent] Missing/non-scalar: ") + path);
    return n.as<std::string>();
}

int AutoAgent::as_int_or_throw(const YAML::Node& n, const char* path) {
    if (!n || !n.IsScalar())
        throw std::runtime_error(std::string("[AutoAgent] Missing/non-scalar: ") + path);
    try { return n.as<int>(); }
    catch (...) { throw std::runtime_error(std::string("[AutoAgent] Bad int at ") + path); }
}

float AutoAgent::as_float_or_throw(const YAML::Node& n, const char* path) {
    if (!n || !n.IsScalar())
        throw std::runtime_error(std::string("[AutoAgent] Missing/non-scalar: ") + path);
    try { return n.as<float>(); }
    catch (...) { throw std::runtime_error(std::string("[AutoAgent] Bad float at ") + path); }
}

// ── load_model ────────────────────────────────────────────────────────────────
void AutoAgent::load_model() {
    const auto& cfg  = Model::get_config()
        ->get_agent_parameters().get_auto_agent();
    const auto& path = cfg.get_model_path();
    model_ = ModelLoader::load_model(path);
    model_.eval();
    model_loaded_ = true;
    spdlog::info("[AutoAgent] Actor TorchScript loaded: {}", path);
}

// ── load_manifest ─────────────────────────────────────────────────────────────
void AutoAgent::load_manifest() {
    const auto& cfg  = Model::get_config()
        ->get_agent_parameters().get_auto_agent();
    const auto& path = cfg.get_manifest_path();
    manifest_ = ModelLoader::load_manifest(path);

    // Window
    const YAML::Node win = manifest_["window"];
    int W = 0;
    if (win && win["W"] && win["W"].IsScalar())
        W = win["W"].as<int>();
    else
        throw std::runtime_error("[AutoAgent] manifest missing window.W");
    if (W != WINDOW)
        throw std::runtime_error(
            "[AutoAgent] manifest window.W=" + std::to_string(W) +
            " does not match compiled WINDOW=" + std::to_string(WINDOW));

    // Allele patterns (ART, PPQ, LUM, AMQ) — order fixed by training feature order
    const YAML::Node ap = manifest_["allele_patterns"];
    if (!ap || !ap.IsMap())
        throw std::runtime_error("[AutoAgent] manifest missing 'allele_patterns' map");

    static const std::vector<std::string> REQUIRED = {"ART","PPQ","LUM","AMQ"};
    allele_names_.clear();
    allele_patterns_.clear();
    for (const auto& name : REQUIRED) {
        if (!ap[name] || !ap[name].IsScalar())
            throw std::runtime_error(
                "[AutoAgent] manifest allele_patterns missing key: " + name);
        allele_names_.push_back(name);
        allele_patterns_.push_back(ap[name].as<std::string>());
    }

    // Reset lazy build
    allele_genotype_ids_.clear();
    allele_genotype_ids_built_  = false;
    allele_genotype_ids_n_geno_ = 0;

    manifest_loaded_ = true;
    spdlog::info("[AutoAgent] Manifest loaded: W={}", W);
    for (size_t a = 0; a < allele_names_.size(); ++a)
        spdlog::info("[AutoAgent]   allele {}='{}'", allele_names_[a], allele_patterns_[a]);
}

// ── initialize ────────────────────────────────────────────────────────────────
void AutoAgent::initialize() {
    load_manifest();
    load_model();

    const auto& cfg = Model::get_config()
        ->get_agent_parameters().get_auto_agent();

    // Beta normalisation
    const float beta_raw = static_cast<float>(
        Model::get_config()->location_db()[0].beta);
    beta_norm_ = std::clamp(
        (beta_raw - BETA_MIN) / (BETA_MAX - BETA_MIN), 0.f, 1.f);
    spdlog::info("[AutoAgent] beta={:.4f} beta_norm={:.4f}", beta_raw, beta_norm_);

    // Trigger date
    const auto& starting = Model::get_config()
        ->get_simulation_timeframe().get_starting_date();
    trigger_day_ = (date::sys_days{cfg.get_trigger_date()} -
                    date::sys_days{starting}).count();
    trigger_month_ = static_cast<int>(trigger_day_) / AA_DAYS_PER_MONTH;
    sim_start_year_ = static_cast<int>(starting.year());

    if (trigger_day_ < 0)
        throw std::runtime_error("[AutoAgent] trigger_date before simulation start");
    if (trigger_month_ < WINDOW)
        throw std::runtime_error(
            "[AutoAgent] trigger_date too early: need >= " +
            std::to_string(WINDOW) + " months of history");

    // Config parameters
    p_mft_threshold_ = cfg.get_p_mft_threshold();
    min_dist_conf_   = cfg.get_min_dist_conf();
    strategy_adc_    = cfg.get_strategy_adc();
    strategy_mft_    = cfg.get_strategy_mft();
    cooldown_months_ = cfg.get_cooldown_months();
    t_active_        = trigger_month_ > 0 ? (trigger_month_ + 1) : 241;

    spdlog::info("[AutoAgent] trigger_day={} trigger_month={} "
                 "p_mft_threshold={:.3f} min_dist_conf={:.3f} "
                 "adc_strategy={} mft_strategy={} cooldown={}mo",
                 trigger_day_, trigger_month_,
                 p_mft_threshold_, min_dist_conf_,
                 strategy_adc_, strategy_mft_, cooldown_months_);

    // Resize per-level data
    const auto* admin   = Model::get_spatial_data()->get_admin_level_manager();
    const int n_levels  = static_cast<int>(admin->get_level_names().size());
    auto_agent_data_by_level.resize(n_levels + 1);

    for (int lv = 0; lv <= n_levels; ++lv) {
        const bool is_cell = (lv == n_levels);
        if (is_cell && n_levels > 0) continue;

        const int vsz = is_cell
            ? Model::get_config()->number_of_locations()
            : [&]() -> int {
                const auto* b = admin->get_boundary(admin->get_level_names()[lv]);
                return b ? b->max_unit_id + 1 : 0;
              }();

        if (vsz <= 0) continue;
        auto& d = auto_agent_data_by_level[lv];
        d.set_history_cap(WINDOW);
        d.reset_month(vsz);
        d.ensure_state_size(vsz);
    }

    spdlog::info("[AutoAgent] Initialized. WINDOW={} STATE_DIM={}", WINDOW, STATE_DIM);
}

// ── AutoAgentData::reset_month ────────────────────────────────────────────────
void AutoAgent::AutoAgentData::reset_month(int n) {
    monthly_new_infections.assign(n, 0.0);
    monthly_treatment.assign(n, 0.0);
    monthly_clinical.assign(n, 0.0);
    current_tf.assign(n, 0.0);
    monthly_tf.assign(n, 0.0);
    monthly_mutation.assign(n, 0.0);
    total_bites.assign(n, 0.0);
    total_bites_year.assign(n, 0.0);
    today_treatments.assign(n, 0.0);
    current_mutation_year.assign(n, 0.0);
    total_immune.assign(n, 0.0);
    tf6.assign(n, 0.0); tf7.assign(n, 0.0); tf8.assign(n, 0.0);
    freq_ART.assign(n, 0.0); freq_PPQ.assign(n, 0.0);
    freq_LUM.assign(n, 0.0); freq_AMQ.assign(n, 0.0);
    popsize.assign(n, 0.0);
    clinical_age.assign(n,  std::vector<double>(N_AGE_SINGLE, 0.0));
    bsp_age_group.assign(n, std::vector<double>(N_AGE_GROUP,  0.0));
    bsp_age.assign(n,       std::vector<double>(N_AGE_SINGLE, 0.0));
    popsize_age.assign(n,   std::vector<double>(N_AGE_SINGLE, 0.0));
    clinical_ag.assign(n,   std::vector<double>(N_AGE_GROUP,  0.0));
    moi.assign(n,           std::vector<double>(AutoAgent::N_MOI, 0.0));
}

// ── AutoAgentData::ensure_state_size ─────────────────────────────────────────
void AutoAgent::AutoAgentData::ensure_state_size(int n) {
    if (static_cast<int>(guard.size()) != n)
        guard.assign(n, GuardState{});
    if (static_cast<int>(current_strategy.size()) != n)
        current_strategy.assign(n, 4);   // default: ADC
}

// ── AutoAgentData::push_month_all ─────────────────────────────────────────────
void AutoAgent::AutoAgentData::push_month_all() {
    const int cap = max_history;

    auto ps = [&](std::deque<std::vector<double>>& q,
                  const std::vector<double>& v) {
        q.push_back(v);
        while (static_cast<int>(q.size()) > cap) q.pop_front();
    };
    auto p2 = [&](std::deque<std::vector<std::vector<double>>>& q,
                  const std::vector<std::vector<double>>& v) {
        q.push_back(v);
        while (static_cast<int>(q.size()) > cap) q.pop_front();
    };

    ps(h_new_infections,       monthly_new_infections);
    ps(h_treatment,            monthly_treatment);
    ps(h_clinical,             monthly_clinical);
    ps(h_current_tf,           current_tf);
    ps(h_monthly_tf,           monthly_tf);
    ps(h_mutation,             monthly_mutation);
    ps(h_total_bites,          total_bites);
    ps(h_total_bites_year,     total_bites_year);
    ps(h_today_treatments,     today_treatments);
    ps(h_current_mutation_year,current_mutation_year);
    ps(h_total_immune,         total_immune);
    ps(h_tf6, tf6); ps(h_tf7, tf7); ps(h_tf8, tf8);
    ps(h_ART, freq_ART); ps(h_PPQ, freq_PPQ);
    ps(h_LUM, freq_LUM); ps(h_AMQ, freq_AMQ);
    ps(h_popsize, popsize);
    p2(h_clinical_age,  clinical_age);
    p2(h_bsp_age_group, bsp_age_group);
    p2(h_bsp_age,       bsp_age);
    p2(h_popsize_age,   popsize_age);
    p2(h_clinical_ag,   clinical_ag);
    p2(h_moi,           moi);

    // Snapshot current therapy distribution per unit from tf6/tf7/tf8
    {
        const int n = static_cast<int>(tf6.size());
        std::vector<DistHistory> dh(n);
        for (int u = 0; u < n; ++u) {
            const float r6 = static_cast<float>(tf6[u]);
            const float r7 = static_cast<float>(tf7[u]);
            const float r8 = static_cast<float>(tf8[u]);
            const float s  = r6 + r7 + r8;
            const float den = (s > 1e-8f) ? s : 1.f;
            dh[u] = {r6/den, r7/den, r8/den};
        }
        h_dist.push_back(std::move(dh));
        while (static_cast<int>(h_dist.size()) > cap) h_dist.pop_front();
    }
}

// ── AutoAgentData::build_state_batch ─────────────────────────────────────────
// Constructs [n_units, STATE_DIM] where STATE_DIM = 24 * 98.
// Each timestep row (98 floats):
//   [0..91]  92 preprocessed features  (see summary.txt section 4b)
//   [92..94] dist_6, dist_7, dist_8
//   [95]     beta_norm
//   [96]     t_pos
//   [97]     strat_flag (1=MFT, 0=ADC)
// ──────────────────────────────────────────────────────────────────────────────
std::vector<std::vector<float>>
AutoAgent::AutoAgentData::build_state_batch(
    float beta_norm, int t_now, int T) const
{
    const int hist = history_len();
    if (hist < AutoAgent::WINDOW)
        throw std::runtime_error("[AutoAgent] build_state_batch: insufficient history");

    const int n_units = static_cast<int>(h_current_tf.back().size());
    if (n_units == 0)
        throw std::runtime_error("[AutoAgent] build_state_batch: empty unit vector");

    const int   W         = AutoAgent::WINDOW;
    const int   SD        = AutoAgent::STATE_DIM;  // W * FEAT_PER_STEP
    const int   t_start   = hist - W;
    const float T_f       = static_cast<float>(std::max(T - 1, 1));

    std::vector<std::vector<float>> batch(n_units, std::vector<float>(SD, 0.f));

    for (int u = 0; u < n_units; ++u) {
        const int   strat      = current_strategy[u];
        const float strat_flag = (strat == 5) ? 1.0f : 0.0f;

        for (int w = 0; w < W; ++w) {
            const int    t       = t_start + w;
            // Absolute timestep index for t_pos: oldest window step = t_now - (W-1)
            const int    t_abs   = t_now - (W - 1 - w);
            const float  t_pos   = std::clamp(
                static_cast<float>(t_abs) / T_f, 0.f, 1.f);
            const float  pop     = static_cast<float>(
                std::max(h_popsize[t][u], 1.0));

            float* row = batch[u].data() + w * AutoAgent::FEAT_PER_STEP;

            // ── [0] monthly_number_of_new_infections — pop-norm then log1p ─
            row[0] = std::log1p(
                std::max(static_cast<float>(h_new_infections[t][u]) / pop, 0.f));

            // ── [1] monthly_number_of_treatment — pop-norm then log1p ──────
            row[1] = std::log1p(
                std::max(static_cast<float>(h_treatment[t][u]) / pop, 0.f));

            // ── [2] monthly_number_of_clinical_episode — pop-norm ──────────
            row[2] = static_cast<float>(h_clinical[t][u]) / pop;

            // ── [3..13] clinical_episode_by_age_{0,1,10,2..9} — pop-norm ──
            for (int k = 0; k < N_AGE_SINGLE; ++k)
                row[3 + k] = static_cast<float>(h_clinical_age[t][u][k]) / pop;

            // ── [14..23] multiple_of_infection_0..9 — raw (already in [0,1])
            for (int k = 0; k < AutoAgent::N_MOI; ++k)
                row[14 + k] = static_cast<float>(h_moi[t][u][k]);

            // ── [24..38] bsp_age_group_{0,1,10,11..14,2..9} — raw ─────────
            for (int k = 0; k < N_AGE_GROUP; ++k)
                row[24 + k] = static_cast<float>(h_bsp_age_group[t][u][k]);

            // ── [39..49] bsp_age_{0,1,10,2..9} — raw ──────────────────────
            for (int k = 0; k < N_AGE_SINGLE; ++k)
                row[39 + k] = static_cast<float>(h_bsp_age[t][u][k]);

            // ── [50] current_TF_by_location — raw ─────────────────────────
            row[50] = static_cast<float>(h_current_tf[t][u]);

            // ── [51] monthly_mutation — pop-norm then log1p ────────────────
            row[51] = std::log1p(
                std::max(static_cast<float>(h_mutation[t][u]) / pop, 0.f));

            // ── [52] total_number_of_bites_by_location — pop-norm ──────────
            row[52] = static_cast<float>(h_total_bites[t][u]) / pop;

            // ── [53] total_number_of_bites_by_location_year — pop-norm ─────
            row[53] = static_cast<float>(h_total_bites_year[t][u]) / pop;

            // ── [54] today_number_of_treatments — pop-norm ─────────────────
            row[54] = static_cast<float>(h_today_treatments[t][u]) / pop;

            // ── [55] current_number_of_mutation_events_in_this_year — pop-norm, log1p
            row[55] = std::log1p(
                std::max(static_cast<float>(h_mutation[t][u]) / pop, 0.f));

            // ── [56] popsize_by_location_hoststate — raw (already used as pop)
            row[56] = pop;

            // ── [57..67] popsize_by_location_age_{0,1,10,2..9} — raw count ─
            for (int k = 0; k < N_AGE_SINGLE; ++k)
                row[57 + k] = static_cast<float>(h_popsize_age[t][u][k]);

            // ── [68..82] number_of_clinical_by_location_age_group — pop-norm
            for (int k = 0; k < N_AGE_GROUP; ++k)
                row[68 + k] = static_cast<float>(h_clinical_ag[t][u][k]) / pop;

            // ── [83] total_immune_by_location — raw ───────────────────────
            row[83] = static_cast<float>(h_total_immune[t][u]);

            // ── [84..86] tf_by_therapy_6/7/8 — raw ───────────────────────
            row[84] = static_cast<float>(h_tf6[t][u]);
            row[85] = static_cast<float>(h_tf7[t][u]);
            row[86] = static_cast<float>(h_tf8[t][u]);

            // ── [87] monthly_number_of_TF — pop-norm ─────────────────────
            row[87] = static_cast<float>(h_monthly_tf[t][u]) / pop;

            // ── [88..91] ART, PPQ, LUM, AMQ allele frequencies — raw ──────
            row[88] = static_cast<float>(h_ART[t][u]);
            row[89] = static_cast<float>(h_PPQ[t][u]);
            row[90] = static_cast<float>(h_LUM[t][u]);
            row[91] = static_cast<float>(h_AMQ[t][u]);

            // ── [92..94] dist_6, dist_7, dist_8 ──────────────────────────
            if (!h_dist.empty() && t < static_cast<int>(h_dist.size())) {
                const auto& dh = h_dist[t][u];
                const float ds = dh.d6 + dh.d7 + dh.d8;
                const float dd = (ds > 1e-8f) ? ds : 1.f;
                row[92] = dh.d6 / dd;
                row[93] = dh.d7 / dd;
                row[94] = dh.d8 / dd;
            } else {
                row[92] = 0.f; row[93] = 0.f; row[94] = 1.f;  // default: all th8
            }

            // ── [95] beta_norm ────────────────────────────────────────────
            row[95] = beta_norm;

            // ── [96] t_pos ────────────────────────────────────────────────
            row[96] = t_pos;

            // ── [97] strat_flag ───────────────────────────────────────────
            row[97] = strat_flag;

            // Guard against NaN/Inf from uninitialised MDC fields
            for (int f = 0; f < AutoAgent::FEAT_PER_STEP; ++f)
                if (!std::isfinite(row[f])) row[f] = 0.f;
        }
    }
    return batch;
}

// ── reset_auto_data ───────────────────────────────────────────────────────────
void AutoAgent::reset_auto_data(int level_id, int vector_size) {
    auto& d = auto_agent_data_by_level[level_id];
    d.ensure_state_size(vector_size);
    d.reset_month(vector_size);
}

// ── build_allele_genotype_ids (private) ───────────────────────────────────────
void AutoAgent::build_allele_genotype_ids(int numGenotypes) {
    auto* gdb = Model::get_genotype_db();
    const int n_geno    = static_cast<int>(gdb->size());
    const int n_alleles = static_cast<int>(allele_patterns_.size());

    if (n_alleles == 0)
        throw std::runtime_error(
            "[AutoAgent] allele_patterns_ empty — was load_manifest() called?");

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
                ++matched[a];
            }
        }
    }

    for (int a = 0; a < n_alleles; ++a) {
        spdlog::info("[AutoAgent] Allele {} pattern='{}' matched {}/{} genotypes",
                     allele_names_[a], allele_patterns_[a], matched[a], n_geno);
        if (matched[a] == 0)
            spdlog::warn("[AutoAgent] Allele {} matched 0 genotypes — "
                         "check pattern against genotype names in your config",
                         allele_names_[a]);
    }

    allele_genotype_ids_built_  = true;
    allele_genotype_ids_n_geno_ = n_geno;
}

// ── finalize_month_all_features ───────────────────────────────────────────────
void AutoAgent::finalize_month_all_features(
    int level_id,
    int numGenotypes,
    const std::vector<SQLiteValidationReporter::MonthlyGenomeData>& genome_data)
{
    auto& d   = auto_agent_data_by_level[level_id];
    const int n = static_cast<int>(d.freq_ART.size());
    if (n == 0) return;

    // ── Lazy-build allele genotype ID sets ────────────────────────────────────
    if (!allele_genotype_ids_built_ ||
        numGenotypes > allele_genotype_ids_n_geno_)
    {
        build_allele_genotype_ids(numGenotypes);
    }

    // ── Compute allele frequencies ────────────────────────────────────────────
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
        if (n_alleles > 0) d.freq_ART[u] = sums[0] / denom;
        if (n_alleles > 1) d.freq_PPQ[u] = sums[1] / denom;
        if (n_alleles > 2) d.freq_LUM[u] = sums[2] / denom;
        if (n_alleles > 3) d.freq_AMQ[u] = sums[3] / denom;
    }

    // ── tf6/7/8 — global rates, read once per finalize call ───────────────────
    for (int u = 0; u < n; ++u) {
        d.tf6[u] = Model::get_mdc()->current_tf_by_therapy()[6];
        d.tf7[u] = Model::get_mdc()->current_tf_by_therapy()[7];
        d.tf8[u] = Model::get_mdc()->current_tf_by_therapy()[8];
    }

    d.push_month_all();

    const int now_day   = Model::get_scheduler()->current_time();
    const int month_abs = now_day / AA_DAYS_PER_MONTH;

    if (d.history_len() >= WINDOW && now_day >= trigger_day_)
        inference_from_auto_data(level_id);
}

// ── inference_from_auto_data ──────────────────────────────────────────────────
void AutoAgent::inference_from_auto_data(int level_id) {
    if (!model_loaded_) {
        spdlog::error("[AutoAgent] inference called before model is loaded");
        return;
    }

    auto& d       = auto_agent_data_by_level[level_id];
    const int now = Model::get_scheduler()->current_time();
    const int t_now = now / AA_DAYS_PER_MONTH - AA_BURN_IN;  // post-burn-in month index

    d.ensure_state_size(static_cast<int>(d.h_current_tf.back().size()));

    // ── Global cooldown / pending-event guard (unit 0 drives the global decision)
    auto& g = d.guard[0];

    // Resolve a pending event that has fired
    if (g.pending_day != -1 && now >= g.pending_day) {
        g.pending_day      = -1;
        g.pending_strategy = -1;
        g.block_until_day  = now + cooldown_months_ * AA_DAYS_PER_MONTH;
        spdlog::info("[AutoAgent] Pending strategy event fired at day={}; "
                     "cooldown until day={}", now, g.block_until_day);
    }

    // Skip if in cooldown or already waiting for a pending event
    if ((g.block_until_day != -1 && now < g.block_until_day) ||
        (g.pending_day     != -1 && g.pending_day > now))
        return;

    // ── Build state tensor ────────────────────────────────────────────────────
    std::vector<std::vector<float>> batch;
    try {
        batch = d.build_state_batch(beta_norm_, t_now, t_active_);
    } catch (const std::exception& e) {
        spdlog::warn("[AutoAgent] build_state_batch: {}", e.what());
        return;
    }

    const int N = static_cast<int>(batch.size());
    if (N == 0) return;

    // NaN / Inf check
    bool has_nan = false;
    for (int b = 0; b < N && !has_nan; ++b)
        for (float v : batch[b])
            if (!std::isfinite(v)) { has_nan = true; break; }
    if (has_nan) {
        spdlog::warn("[AutoAgent] month={} input contains NaN/Inf — skipping inference",
                     t_now + AA_BURN_IN);
        return;
    }

    // Build flat tensor [N, STATE_DIM]
    auto x = torch::zeros({N, STATE_DIM}, torch::kFloat32);
    for (int b = 0; b < N; ++b)
        for (int i = 0; i < STATE_DIM; ++i)
            x[b][i] = batch[b][i];

    // ── Model forward pass ────────────────────────────────────────────────────
    torch::Tensor out;
    try {
        torch::NoGradGuard no_grad;
        // TorchScript actor: forward(raw_state: Tensor) -> Tensor [B, 4]
        // Normalization (x-mu)/sd is BAKED INTO the TorchScript graph.
        out = model_.forward({x}).toTensor();  // [N, 4]
    } catch (const std::exception& e) {
        spdlog::error("[AutoAgent] actor forward failed: {}", e.what());
        return;
    }

    // ── Extract unit-0 (global) action ────────────────────────────────────────
    //   col 0  p_mft  → strategy decision
    //   col 1  d_th6 \
    //   col 2  d_th7  ├→ therapy distribution
    //   col 3  d_th8 /
    Action a;
    a.p_mft = out[0][0].item<float>();
    a.d_th6 = out[0][1].item<float>();
    a.d_th7 = out[0][2].item<float>();
    a.d_th8 = out[0][3].item<float>();
    a.strategy_id = (a.p_mft >= p_mft_threshold_) ? strategy_mft_ : strategy_adc_;

    // Dominant therapy (argmax of dist)
    if      (a.d_th8 >= a.d_th7 && a.d_th8 >= a.d_th6) a.dominant_therapy = 8;
    else if (a.d_th7 >= a.d_th6)                        a.dominant_therapy = 7;
    else                                                  a.dominant_therapy = 6;

    spdlog::info(
        "[AutoAgent] month={} p_mft={:.3f} strategy={} "
        "dist=[th6={:.3f} th7={:.3f} th8={:.3f}] dominant_th={}",
        t_now + AA_BURN_IN, a.p_mft, a.strategy_id,
        a.d_th6, a.d_th7, a.d_th8, a.dominant_therapy);

    // ── Confidence gate ───────────────────────────────────────────────────────
    // Require the dominant therapy to exceed min_dist_conf before committing.
    const float max_dist = std::max({a.d_th6, a.d_th7, a.d_th8});
    if (max_dist < min_dist_conf_) {
        spdlog::info(
            "[AutoAgent] Dominant therapy confidence {:.3f} < {:.3f} — no switch",
            max_dist, min_dist_conf_);
        return;
    }

    // ── Check whether the proposed strategy is already active ─────────────────
    const int current_strat = d.current_strategy[0];
    if (a.strategy_id == current_strat) {
        spdlog::info("[AutoAgent] Proposed strategy {} is already active — skip",
                     a.strategy_id);
        return;
    }

    // ── Schedule the strategy change ──────────────────────────────────────────
    // Emit the event at now + WINDOW months (same horizon as ADC agent).
    const int forecast_day = now + WINDOW * AA_DAYS_PER_MONTH;

    auto event = std::make_unique<ChangeTreatmentStrategyEvent>(
        a.strategy_id, forecast_day);
    event->set_executable(true);
    Model::get_scheduler()->schedule_population_event(std::move(event));

    spdlog::info(
        "[AutoAgent] Scheduled strategy={} (dominant_th={}) at day={} (+{}mo)",
        a.strategy_id, a.dominant_therapy, forecast_day, WINDOW);

    // Update guard and current strategy optimistically
    g.pending_day      = forecast_day;
    g.pending_strategy = a.strategy_id;
    d.current_strategy[0] = a.strategy_id;
}
