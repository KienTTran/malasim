#ifndef AUTOAGENT_H
#define AUTOAGENT_H

/**
 * AutoAgent.h  —  Offline-RL IQL Actor integration  (machine-A v1)
 * ─────────────────────────────────────────────────────────────────
 * Loads the TorchScript actor exported from the IQL (Implicit Q-Learning)
 * training run and uses it to decide, each month:
 *   • WHICH strategy to apply  (ADC=4 or MFT=5, via p_mft threshold)
 *   • WHAT therapy distribution to target  (th6, th7, th8)
 *
 * Model interface  (actor_torchscript.pt, normalization BAKED IN):
 *   Input:  raw_state  [B, STATE_DIM=2352]  (flattened [24 × 98])
 *   Output: [B, 4]
 *     col 0  p_mft   P(strategy=MFT=5),  range [0,1]
 *     col 1  d_th6   softmax dist fraction
 *     col 2  d_th7   softmax dist fraction
 *     col 3  d_th8   softmax dist fraction
 *
 * State layout  [24 timesteps × 98 values] — oldest first:
 *   cols  [0..91]  92 preprocessed simulation features  (section 4b of summary)
 *   cols  [92..94] dist_6, dist_7, dist_8               (current therapy fractions)
 *   col   [95]     beta_norm = (beta - 0.089) / (0.630 - 0.089)
 *   col   [96]     t_pos     = timestep / (T-1)         (relative timeline position)
 *   col   [97]     strat_flag = 1.0 if strategy==MFT(5), else 0.0
 *
 * Feature preprocessing applied BEFORE building the state vector:
 *   Step 1 — population-normalise count features (prefix monthly_number_of_, etc.)
 *   Step 2 — log1p(x) for mutation / treatment / new-infection sub-features
 *   Step 3 — dist_k /= sum(dist_6+dist_7+dist_8)   (clamp denom ≥ 1e-8)
 *   Features already in [0,1] (BSP, TF, allele freqs) are passed through raw.
 *
 * Age sub-indices stored per slot (matching NPZ ordering):
 *   clinical_age     [u][0..10]  → MDC age indices  {0,1,10,2,3,4,5,6,7,8,9}
 *   bsp_age_group    [u][0..14]  → MDC age-group    {0,1,10,11,12,13,14,2,3,4,5,6,7,8,9}
 *   bsp_age          [u][0..10]  → MDC age indices  {0,1,10,2,3,4,5,6,7,8,9}
 *   multiple_of_inf  [u][0..9]   → MOI bucket       0..9
 *
 * MDC calls required per location in collect_site_data_for_location():
 *   monthly_number_of_new_infections_by_location()[loc]
 *   monthly_number_of_treatment_by_location()[loc]
 *   monthly_number_of_clinical_episode_by_location()[loc]
 *   monthly_number_of_clinical_episode_by_location_age()[loc][age]  age 0..10
 *   blood_slide_prevalence_by_location_age_group()[loc][ag]          ag  0..14
 *   blood_slide_prevalence_by_location_age()[loc][age]               age 0..10
 *   multiple_of_infection()[loc][moi]                                moi 0..9
 *   current_tf_by_location()[loc]
 *   monthly_number_of_tf_by_location()[loc]
 *   monthly_number_of_mutation_events_by_location()[loc]
 *   total_number_of_bites_by_location()[loc]
 *   total_number_of_bites_by_location_year()[loc]
 *   today_number_of_treatments_by_location()[loc]      (daily snapshot used for current_number_of_treatments)
 *   current_number_of_mutation_events_in_this_year()[loc]
 *   popsize_by_location_hoststate()[loc]
 *   popsize_by_location_age()[loc][age]                age 0..10
 *   number_of_clinical_by_location_age_group()[loc][ag] ag 0..14
 *   total_immune_by_location()[loc]
 *   current_tf_by_therapy()[6], [7], [8]
 *
 * Config YAML block consumed by AutoAgent:
 *   agent_parameters:
 *     auto_agent:
 *       enabled:          true
 *       model_path:       ../agent_models/actor_torchscript.pt
 *       manifest_path:    ../agent_models/auto_agent_manifest.yml
 *       trigger_date:     2010/1/1
 *       p_mft_threshold:  0.5     # >= this → use MFT(5), else ADC(4)
 *       min_dist_conf:    0.60    # dominant therapy fraction must exceed this
 *       strategy_adc:     4       # strategy_id to activate for ADC
 *       strategy_mft:     5       # strategy_id to activate for MFT
 *       cooldown_months:  24
 *
 * The manifest YAML must contain:
 *   allele_patterns: { ART: '<regex>', PPQ: '<regex>', LUM: '<regex>', AMQ: '<regex>' }
 *   window: { W: 24 }
 *
 * Relation to AdaptiveCyclingAgent:
 *   AutoAgent is a SEPARATE class that consumes the IQL actor model instead of
 *   the v5.5 transformer.  Both can co-exist in the same binary; they are
 *   enabled/disabled independently via their config keys.
 */

#include <torch/script.h>
#include <yaml-cpp/yaml.h>

#include <array>
#include <cmath>
#include <deque>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ModelLoader.h"
#include "Reporters/Reporter.h"
#include "Reporters/SQLiteValidationReporter.h"

class AutoAgent {
public:
    // ── Model constants (must match training script / summary.txt) ────────────
    static constexpr int   WINDOW        = 24;     ///< months in the rolling window
    static constexpr int   N_FEATURES    = 92;     ///< YAML-selected features per timestep
    static constexpr int   N_DIST        = 3;      ///< therapy dist columns (th6,th7,th8)
    static constexpr int   FEAT_PER_STEP = 98;     ///< total columns per timestep
    static constexpr int   STATE_DIM     = 2352;   ///< WINDOW * FEAT_PER_STEP
    static constexpr int   N_MOI         = 10;     ///< MOI bucket count (0..9)
    static constexpr float BETA_MIN      = 0.089f;
    static constexpr float BETA_MAX      = 0.630f;

    // ── Per-unit action output ────────────────────────────────────────────────
    struct Action {
        float p_mft;        ///< P(strategy == MFT), range [0,1]
        int   strategy_id;  ///< 4 (ADC) or 5 (MFT), decided by p_mft_threshold
        float d_th6;        ///< therapy 6 fraction, softmax output
        float d_th7;        ///< therapy 7 fraction, softmax output
        float d_th8;        ///< therapy 8 fraction, softmax output
        int   dominant_therapy; ///< 6, 7, or 8 — argmax(d_th6, d_th7, d_th8)
    };

    // ── Guard state — prevents rapid re-triggering ────────────────────────────
    struct GuardState {
        int pending_day      = -1;   ///< day on which the pending event fires
        int pending_strategy = -1;
        int block_until_day  = -1;   ///< cooldown ends at this day
    };

    // ── Therapy distribution history per unit (for strat_flag tracking) ───────
    struct DistHistory {
        float d6 = 0.f, d7 = 0.f, d8 = 1.f;  ///< normalised at each month
    };

    // ── Per-level rolling data ────────────────────────────────────────────────
    struct AutoAgentData {
        // ── Monthly scalars (reset each month, accumulated over locations) ──
        std::vector<double> monthly_new_infections;
        std::vector<double> monthly_treatment;
        std::vector<double> monthly_clinical;
        std::vector<double> current_tf;
        std::vector<double> monthly_tf;
        std::vector<double> monthly_mutation;
        std::vector<double> total_bites;          ///< total_number_of_bites_by_location
        std::vector<double> total_bites_year;     ///< total_number_of_bites_by_location_year
        std::vector<double> today_treatments;     ///< today_number_of_treatments_by_location
        std::vector<double> current_mutation_year;///< current_number_of_mutation_events_in_this_year
        std::vector<double> total_immune;         ///< total_immune_by_location
        std::vector<double> tf6, tf7, tf8;        ///< current_tf_by_therapy()[6/7/8]
        std::vector<double> freq_ART, freq_PPQ;
        std::vector<double> freq_LUM, freq_AMQ;
        std::vector<double> popsize;              ///< popsize_by_location_hoststate

        // ── 2-D scalars [unit][sub-index] ─────────────────────────────────
        std::vector<std::vector<double>> clinical_age;    ///< [u][11]  age 0..10
        std::vector<std::vector<double>> bsp_age_group;   ///< [u][15]  ag  0..14
        std::vector<std::vector<double>> bsp_age;         ///< [u][11]  age 0..10
        std::vector<std::vector<double>> popsize_age;     ///< [u][11]  age 0..10
        std::vector<std::vector<double>> clinical_ag;     ///< [u][15]  ag  0..14
        std::vector<std::vector<double>> moi;             ///< [u][10]  bucket 0..9

        // ── Deque history (one entry per month, capped to max_history) ─────
        std::deque<std::vector<double>> h_new_infections;
        std::deque<std::vector<double>> h_treatment;
        std::deque<std::vector<double>> h_clinical;
        std::deque<std::vector<double>> h_current_tf;
        std::deque<std::vector<double>> h_monthly_tf;
        std::deque<std::vector<double>> h_mutation;
        std::deque<std::vector<double>> h_total_bites;
        std::deque<std::vector<double>> h_total_bites_year;
        std::deque<std::vector<double>> h_today_treatments;
        std::deque<std::vector<double>> h_current_mutation_year;
        std::deque<std::vector<double>> h_total_immune;
        std::deque<std::vector<double>> h_tf6, h_tf7, h_tf8;
        std::deque<std::vector<double>> h_ART, h_PPQ, h_LUM, h_AMQ;
        std::deque<std::vector<double>> h_popsize;
        std::deque<std::vector<std::vector<double>>> h_clinical_age;    ///< [m][u][11]
        std::deque<std::vector<std::vector<double>>> h_bsp_age_group;   ///< [m][u][15]
        std::deque<std::vector<std::vector<double>>> h_bsp_age;         ///< [m][u][11]
        std::deque<std::vector<std::vector<double>>> h_popsize_age;     ///< [m][u][11]
        std::deque<std::vector<std::vector<double>>> h_clinical_ag;     ///< [m][u][15]
        std::deque<std::vector<std::vector<double>>> h_moi;             ///< [m][u][10]

        // Therapy dist history per unit — updated each month, used to fill
        // the [92..94] dist columns of the state vector.
        std::deque<std::vector<DistHistory>> h_dist; ///< [m][u]

        // Current active strategy per unit (4=ADC, 5=MFT)
        std::vector<int> current_strategy;

        // Guard state per unit (unit 0 drives global decision)
        std::vector<GuardState> guard;

        int max_history = 0;
        int history_len() const { return static_cast<int>(h_current_tf.size()); }
        void set_history_cap(int W) { max_history = W + 2; }

        void reset_month(int n);
        void ensure_state_size(int n);
        void push_month_all();

        /// Build the flattened [n_units, STATE_DIM] input batch.
        std::vector<std::vector<float>> build_state_batch(
            float beta_norm, int t_now, int T) const;
    };

    // ── Per-level data store ──────────────────────────────────────────────────
    std::vector<AutoAgentData> auto_agent_data_by_level;

private:
    // ── TorchScript model ─────────────────────────────────────────────────────
    torch::jit::Module model_;
    YAML::Node         manifest_;
    bool               model_loaded_    = false;
    bool               manifest_loaded_ = false;

    // ── Agent config (read from simulation YAML) ──────────────────────────────
    float  beta_norm_        = 0.f;
    long   trigger_day_      = 1;
    int    trigger_month_    = -1;
    int    sim_start_year_   = 2000;
    int    t_active_         = 241;  ///< post-burn-in run length in months
    float  p_mft_threshold_  = 0.5f;
    float  min_dist_conf_    = 0.60f;
    int    strategy_adc_     = 4;
    int    strategy_mft_     = 5;
    int    cooldown_months_  = 24;

    // ── Allele support ────────────────────────────────────────────────────────
    std::vector<std::string>              allele_names_;
    std::vector<std::string>              allele_patterns_;
    std::vector<std::unordered_set<int>>  allele_genotype_ids_;
    bool allele_genotype_ids_built_  = false;
    int  allele_genotype_ids_n_geno_ = 0;

public:
    // ── Non-copyable / non-movable ────────────────────────────────────────────
    AutoAgent(const AutoAgent&)            = delete;
    AutoAgent& operator=(const AutoAgent&) = delete;
    AutoAgent(AutoAgent&&)                 = delete;
    AutoAgent& operator=(AutoAgent&&)      = delete;

    explicit AutoAgent();
    virtual ~AutoAgent() = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    void load_model();
    void load_manifest();
    void initialize();

    YAML::Node get_manifest() const { return manifest_; }

    // ── Called by SQLiteValidationReporter ───────────────────────────────────
    void reset_auto_data(int level_id, int vector_size);

    /// Computes allele frequencies from genome data and pushes month history.
    /// Triggers inference if enough history and past trigger_day.
    void finalize_month_all_features(
        int level_id,
        int numGenotypes,
        const std::vector<SQLiteValidationReporter::MonthlyGenomeData>&
            monthly_genome_data_by_level);

    /// Runs the actor model and schedules strategy / therapy events if warranted.
    void inference_from_auto_data(int level_id);

    // ── Utilities ─────────────────────────────────────────────────────────────
    static std::string as_str_or_throw(const YAML::Node& n, const char* path);
    static int         as_int_or_throw(const YAML::Node& n, const char* path);
    static float       as_float_or_throw(const YAML::Node& n, const char* path);

private:
    void build_allele_genotype_ids(int numGenotypes);
};

#endif // AUTOAGENT_H
