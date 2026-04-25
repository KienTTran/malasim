#ifndef WORLDMODELAGENT_H
#define WORLDMODELAGENT_H

/**
 * WorldModelAgent.h — World Model MPC integration
 * ────────────────────────────────────────────────────────────────────
 * Model interface (TorchScript, traced MLP):
 *   Input:  (B, n_input) where n_input = n_state*2 + 3
 *           = [state_normalized, delta_normalized, action]
 *   Output: (B, n_state) = predicted delta (normalized)
 *
 * State features (13, from features_v5_max_res_v2.yml):
 *   f[ 0]  current_TF_by_location                raw rate
 *   f[ 1]  monthly_number_of_TF_by_location      raw count
 *   f[ 2]  monthly_number_of_clinical_episode_by_location   raw count
 *   f[ 3]  monthly_number_of_mutation_events_by_location    raw count
 *   f[ 4]  monthly_number_of_new_infections_by_location     raw count
 *   f[ 5]  monthly_number_of_treatment_by_location          raw count
 *   f[ 6]  tf_by_therapy_6                        raw rate
 *   f[ 7]  tf_by_therapy_7                        raw rate
 *   f[ 8]  tf_by_therapy_8                        raw rate
 *   f[ 9]  ART   allele freq
 *   f[10]  PPQ   allele freq
 *   f[11]  LUM   allele freq
 *   f[12]  AMQ   allele freq
 *
 * Planning: at each decision point, roll out 3 therapies for H months,
 * score trajectories by weighted TF + allele frequencies, pick the best.
 * Hysteresis: only switch if alternative is >= switch_threshold% better.
 */

#include <torch/script.h>
#include <yaml-cpp/yaml.h>

#include <array>
#include <cmath>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include "ModelLoader.h"
#include "Reporters/Reporter.h"
#include "Reporters/SQLiteValidationReporter.h"

class WorldModelAgent {
public:
    // ── Configuration ─────────────────────────────────────────────────────────
    static constexpr int N_STATE  = 24;   // state features (from features_v5_max_res_v2.yml)
    static constexpr int N_ACTION = 3;    // [dist_6, dist_7, dist_8]
    static constexpr int N_INPUT  = N_STATE * 2 + N_ACTION;  // 51

    // Feature indices within state vector (must match training order from norm_stats)
    static constexpr int IDX_TF          = 0;   // current_TF_by_location
    static constexpr int IDX_TF_COUNT    = 1;   // monthly_number_of_TF_by_location
    static constexpr int IDX_CLINICAL    = 2;   // monthly_number_of_clinical_episode_by_location
    // IDX 3..13: monthly_number_of_clinical_episode_by_location_age_0..9,10
    static constexpr int IDX_MUTATION    = 14;  // monthly_number_of_mutation_events_by_location
    static constexpr int IDX_INFECTIONS  = 15;  // monthly_number_of_new_infections_by_location
    static constexpr int IDX_TREATMENT   = 16;  // monthly_number_of_treatment_by_location
    static constexpr int IDX_TF6         = 17;  // tf_by_therapy_6
    static constexpr int IDX_TF7         = 18;  // tf_by_therapy_7
    static constexpr int IDX_TF8         = 19;  // tf_by_therapy_8
    static constexpr int IDX_ART         = 20;  // ART allele freq
    static constexpr int IDX_PPQ         = 21;  // PPQ allele freq
    static constexpr int IDX_LUM         = 22;  // LUM allele freq
    static constexpr int IDX_AMQ         = 23;  // AMQ allele freq

    // Therapy IDs
    static constexpr int THERAPY_6 = 6;
    static constexpr int THERAPY_7 = 7;
    static constexpr int THERAPY_8 = 8;

private:
    // ── Model ─────────────────────────────────────────────────────────────────
    torch::jit::Module model_;

    // Normalization stats (loaded from norm_stats.npz)
    std::array<float, N_STATE> state_mean_{};
    std::array<float, N_STATE> state_std_{};
    std::array<float, N_STATE> delta_mean_{};
    std::array<float, N_STATE> delta_std_{};
    std::array<float, N_STATE> target_mean_{};
    std::array<float, N_STATE> target_std_{};

    // ── Planning config ───────────────────────────────────────────────────────
    int    planning_horizon_   = 12;     // months to roll out
    float  discount_           = 0.97f;  // scoring discount per step
    float  switch_threshold_   = 0.05f;  // only switch if X% better
    float  w_tf_   = 1.0f;              // scoring weight: TF
    float  w_art_  = 0.3f;              // scoring weight: ART
    float  w_ppq_  = 0.3f;              // scoring weight: PPQ
    float  w_lum_  = 0.3f;              // scoring weight: LUM
    float  w_amq_  = 0.3f;              // scoring weight: AMQ

    // ── Persistent state ──────────────────────────────────────────────────────
    std::array<float, N_STATE> prev_state_{};  // state from previous month
    bool   has_prev_state_     = false;
    int    current_therapy_    = -1;      // currently deployed therapy (6/7/8), -1 = none
    int    cooldown_remaining_ = 0;       // months remaining before next decision

    // ── Allele regex ──────────────────────────────────────────────────────────
    std::vector<std::string> allele_names_;
    std::vector<std::string> allele_patterns_;
    std::vector<std::unordered_set<int>> allele_genotype_ids_;
    bool allele_ids_built_  = false;
    int  allele_ids_n_geno_ = 0;

    // ── Therapy → strategy mapping ────────────────────────────────────────────
    std::unordered_map<int, int> therapy_to_strategy_;
    long trigger_day_  = 1;
    int  cooldown_months_ = 6;

    // ── Current month data (reset each month) ─────────────────────────────────
    std::array<float, N_STATE> current_state_{};

    // ── Helpers ───────────────────────────────────────────────────────────────
    void normalize_state(const float* raw, float* out) const;
    void normalize_delta(const float* raw, float* out) const;
    void denormalize_target(const float* norm, float* out) const;

    // Run model forward: (state_norm, delta_norm, action) → predicted_delta_norm
    void forward(const float* state_n, const float* delta_n,
                 const float* action, float* pred_delta_n);

    // Score a predicted trajectory
    float score_trajectory(const std::vector<std::array<float, N_STATE>>& trajectory) const;

    // Plan: roll out each therapy, return best therapy_id
    int plan(const float* state_raw, const float* delta_raw);

public:
    WorldModelAgent();
    ~WorldModelAgent() = default;

    WorldModelAgent(const WorldModelAgent&) = delete;
    WorldModelAgent& operator=(const WorldModelAgent&) = delete;

    void load_model();
    void load_norm_stats();
    void load_manifest();
    void initialize();

    // Called each month from the reporter
    void reset_month_data();

    // Accumulate allele frequencies from genome data
    void finalize_month_features(
        int numGenotypes,
        const std::vector<SQLiteValidationReporter::MonthlyGenomeData>& genome_data,
        int level_id
    );

    // Run planning and schedule therapy switch if needed
    void run_inference();
};

#endif // WORLDMODELAGENT_H
