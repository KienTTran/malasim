#ifndef ADAPTIVECYCLINGAGENT_H
#define ADAPTIVECYCLINGAGENT_H

/**
 * AdaptiveCyclingAgent.h  —  v5.5 Transformer integration
 * ─────────────────────────────────────────────────────────
 * Model interface (predict_cpp in TorchScript):
 *   Input:  (B, W=24, F=55)
 *   Output: tuple(dist[B,3], switch_prob[B])
 *     dist[0..2] = [d6, d7, d8], sums to 1
 *     switch_prob = P(dominant therapy changes at T+24)
 *
 * Exact 50 YAML features (from checkpoint, in order):
 *   f[ 0]  monthly_number_of_new_infections_by_location      log1p(v/pop)
 *   f[ 1]  monthly_number_of_treatment_by_location           log1p(v/pop)
 *   f[ 2]  monthly_number_of_clinical_episode_by_location    v/pop
 *   f[ 3..13]  monthly_number_of_clinical_episode_by_location_age_{0,1,10,2,3,4,5,6,7,8,9}  v/pop
 *   f[14..28]  blood_slide_prevalence_by_location_age_group_{0,1,10,11,12,13,14,2,3,4,5,6,7,8,9}
 *   f[29..39]  blood_slide_prevalence_by_location_age_{0,1,10,2,3,4,5,6,7,8,9}
 *   f[40]  current_TF_by_location                            raw rate
 *   f[41]  monthly_number_of_mutation_events_by_location     log1p(v/pop)
 *   f[42]  tf_by_therapy_6                                   raw rate
 *   f[43]  tf_by_therapy_7                                   raw rate
 *   f[44]  tf_by_therapy_8                                   raw rate
 *   f[45]  monthly_number_of_TF_by_location                  v/pop
 *   f[46]  ART   allele freq
 *   f[47]  PPQ   allele freq
 *   f[48]  LUM   allele freq
 *   f[49]  AMQ   allele freq
 *   f[50..52]  switch-timing (computed, not MDC)
 *   f[53]  beta_norm
 *   f[54]  t_pos
 *
 * Age sub-indices present in NPZ (NOT the full range):
 *   _age:       0,1,2,3,4,5,6,7,8,9,10   (11 values)
 *   _age_group: 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14  (15 values)
 *
 * MDC calls required in collect_site_data_for_location():
 *   monthly_number_of_new_infections_by_location()[loc]
 *   monthly_number_of_treatment_by_location()[loc]
 *   monthly_number_of_clinical_episode_by_location()[loc]
 *   monthly_number_of_clinical_episode_by_location_age()[loc][age]  age 0..10
 *   blood_slide_prevalence_by_location_age_group()[loc][ag]          ag  0..14
 *   blood_slide_prevalence_by_location_age()[loc][age]              age 0..10
 *   current_tf_by_location()[loc]
 *   monthly_number_of_tf_by_location()[loc]
 *   monthly_number_of_mutation_events_by_location()[loc]
 *   current_tf_by_therapy()[6], [7], [8]
 *   popsize_by_location()[loc]   (for per-capita norm, not a model feature)
 *
 * Genotype alleles in finalize_month_all_features() via weighted_occurrences:
 *   ART: is_matched_genotype_by_id(g, 12, 10, 'Y')
 *   PPQ: is_matched_genotype_by_id(g,  7,  0, 'x')
 *   LUM: is_matched_genotype_by_id(g,  0,  0, 'Y')
 *   AMQ: is_matched_genotype_by_id(g,  0,  1, 'F')
 *   Verify chr/gene against your GenotypeParameters.
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

class AdaptiveCyclingAgent {
private:
    struct ADCTorchMeta {
        int W = 0;
        int F = 0;
        std::vector<std::string> input_features;
    };

    struct ADCTorchModel {
        ADCTorchMeta       meta;
        torch::jit::Module module;
        torch::Device      device = torch::kCPU;
    };

    torch::jit::Module agent_raw_model_;
    YAML::Node         agent_raw_manifest_;
    ADCTorchMeta       agent_meta_;
    ADCTorchModel      agent_model_;

public:
    // ── Per-unit persistent state ─────────────────────────────────────────────
    struct SwitchState {
        int   last_switch_t = -1;
        float tf_at_switch  = 0.f;
        float mean_interval = 0.f;
        int   n_intervals   = 0;
        int   prev_smoothed = -1;
        std::vector<int> raw_buf;   // rolling argmax(dist) for majority-vote smoothing
    };

    struct GuardState {
        int pending_day      = -1;
        int pending_strategy = -1;
        int block_until_day  = -1;
    };

    // ── Monthly data per level ────────────────────────────────────────────────
    struct ADCAgentData {

        // Scalar accumulators — reset each month, accumulated over locations
        std::vector<double> monthly_new_infections;  // MDC: monthly_number_of_new_infections_by_location
        std::vector<double> monthly_treatment;       // MDC: monthly_number_of_treatment_by_location
        std::vector<double> monthly_clinical;        // MDC: monthly_number_of_clinical_episode_by_location
        std::vector<double> current_tf;              // MDC: current_tf_by_location
        std::vector<double> monthly_tf;              // MDC: monthly_number_of_tf_by_location
        std::vector<double> monthly_mutation;        // MDC: monthly_number_of_mutation_events_by_location
        std::vector<double> tf6, tf7, tf8;           // MDC: current_tf_by_therapy()[6/7/8]
        std::vector<double> freq_ART, freq_PPQ;
        std::vector<double> freq_LUM, freq_AMQ;
        std::vector<double> popsize;                 // MDC: popsize_by_location (norm only)

        // 2-D accumulators [unit][sub-index]
        // clinical_age[unit][0..10]: MDC monthly_clinical_episode_by_location_age, ages 0..10
        std::vector<std::vector<double>> clinical_age;   // [unit][11]
        // bsp_age_group[unit][0..14]: MDC blood_slide_prevalence_by_location_age_group, groups 0..14
        std::vector<std::vector<double>> bsp_age_group;  // [unit][15]
        // bsp_age[unit][0..10]: MDC blood_slide_prevalence_by_location_age, ages 0..10
        std::vector<std::vector<double>> bsp_age;        // [unit][11]

        // Deque history — one entry per month, trimmed to max_history
        std::deque<std::vector<double>> h_new_infections;
        std::deque<std::vector<double>> h_treatment;
        std::deque<std::vector<double>> h_clinical;
        std::deque<std::vector<double>> h_current_tf;
        std::deque<std::vector<double>> h_monthly_tf;
        std::deque<std::vector<double>> h_mutation;
        std::deque<std::vector<double>> h_tf6, h_tf7, h_tf8;
        std::deque<std::vector<double>> h_ART, h_PPQ, h_LUM, h_AMQ;
        std::deque<std::vector<double>> h_popsize;
        std::deque<std::vector<std::vector<double>>> h_clinical_age;   // [m][u][0..10]
        std::deque<std::vector<std::vector<double>>> h_bsp_age_group;  // [m][u][0..14]
        std::deque<std::vector<std::vector<double>>> h_bsp_age;        // [m][u][0..10]

        // Persistent state
        std::vector<SwitchState> switch_state;
        std::vector<GuardState>  guard;

        int max_history = 0;
        int history_len() const { return static_cast<int>(h_current_tf.size()); }
        void set_history_cap(int W) { max_history = W + 2; }

        void reset_month(int n) {
            monthly_new_infections.assign(n, 0.0);
            monthly_treatment.assign(n, 0.0);
            monthly_clinical.assign(n, 0.0);
            current_tf.assign(n, 0.0);
            monthly_tf.assign(n, 0.0);
            monthly_mutation.assign(n, 0.0);
            tf6.assign(n, 0.0); tf7.assign(n, 0.0); tf8.assign(n, 0.0);
            freq_ART.assign(n, 0.0); freq_PPQ.assign(n, 0.0);
            freq_LUM.assign(n, 0.0); freq_AMQ.assign(n, 0.0);
            popsize.assign(n, 0.0);
            clinical_age.assign(n,  std::vector<double>(11, 0.0));
            bsp_age_group.assign(n, std::vector<double>(15, 0.0));
            bsp_age.assign(n,       std::vector<double>(11, 0.0));
        }

        void ensure_state_size(int n) {
            if (static_cast<int>(guard.size())        != n) guard.assign(n,        GuardState{});
            if (static_cast<int>(switch_state.size()) != n) switch_state.assign(n, SwitchState{});
        }

        void push_month_all() {
            const int cap = max_history;
            auto ps = [&](std::deque<std::vector<double>>& q, const std::vector<double>& v) {
                q.push_back(v);
                while (static_cast<int>(q.size()) > cap) q.pop_front();
            };
            auto p2 = [&](std::deque<std::vector<std::vector<double>>>& q,
                           const std::vector<std::vector<double>>& v) {
                q.push_back(v);
                while (static_cast<int>(q.size()) > cap) q.pop_front();
            };
            ps(h_new_infections, monthly_new_infections);
            ps(h_treatment,      monthly_treatment);
            ps(h_clinical,       monthly_clinical);
            ps(h_current_tf,     current_tf);
            ps(h_monthly_tf,     monthly_tf);
            ps(h_mutation,       monthly_mutation);
            ps(h_tf6, tf6); ps(h_tf7, tf7); ps(h_tf8, tf8);
            ps(h_ART, freq_ART); ps(h_PPQ, freq_PPQ);
            ps(h_LUM, freq_LUM); ps(h_AMQ, freq_AMQ);
            ps(h_popsize, popsize);
            p2(h_clinical_age,  clinical_age);
            p2(h_bsp_age_group, bsp_age_group);
            p2(h_bsp_age,       bsp_age);
        }

        // Build (N_units, W*F) input batch for the model
        std::vector<std::vector<float>> build_input_batch(
            int W, int F,
            float beta_norm,
            int month_abs, int burn_in, int t_active
        ) const;
    };

    std::vector<ADCAgentData> adc_agent_data_by_level;

    // ── Helpers ───────────────────────────────────────────────────────────────
    static int         as_int_or_throw(const YAML::Node& n, const char* path);
    static std::string as_str_or_throw(const YAML::Node& n, const char* path);

    static inline float sigmoidf(float x) {
        return x >= 0.f ? 1.f / (1.f + std::exp(-x))
                        : std::exp(x) / (1.f + std::exp(x));
    }

    static int  smoothed_therapy(SwitchState& sw, int raw_argmax);
    static void update_switch_state(SwitchState& sw, int t_pb, int sm, float tf);
    static std::array<float,3> switch_feats(int t_pb, const SwitchState& sw);

    // ── Agent parameters ──────────────────────────────────────────────────────
    double trigger_value_   = 0.7;
    long   trigger_day_     = 1;
    int    trigger_month_   = -1;
    int    sim_start_year_  = 2000;
    int    sim_start_month_ = 0;
    float  beta_norm_       = 0.f;

    // therapy_id (6/7/8) → strategy_id
    std::unordered_map<int,int> therapy_to_strategy_;

    // Pre-computed genotype ID sets for allele frequency calculation.
    // Built lazily on first call to finalize_month_all_features() once the
    // full genotype DB is populated. Rebuilt if new genotypes are added.
    // Index: 0=ART, 1=PPQ, 2=LUM, 3=AMQ
    std::unordered_set<int> allele_genotype_ids_[4];
    bool allele_genotype_ids_built_  = false;
    int  allele_genotype_ids_n_geno_ = 0;

    // Allele regex patterns (can be overridden in the ADC manifest)
    // Default patterns match the original hard-coded behavior.
    // Index mapping: 0=ART, 1=PPQ, 2=LUM, 3=AMQ
    std::array<std::string,4> allele_patterns_ = {
        std::string(".*PRPYRA\\|.*"),   // ART (index 0)
        std::string(".*\\|2$"),         // PPQ (index 1)
        std::string("^.{4}NY.{3}K"),      // LUM (index 2)
        std::string("^.{4}YY.{3}T")       // AMQ (index 3)
    };

public:
    AdaptiveCyclingAgent(const AdaptiveCyclingAgent&)            = delete;
    AdaptiveCyclingAgent& operator=(const AdaptiveCyclingAgent&) = delete;
    AdaptiveCyclingAgent(AdaptiveCyclingAgent&&)                 = delete;
    AdaptiveCyclingAgent& operator=(AdaptiveCyclingAgent&&)      = delete;

    explicit AdaptiveCyclingAgent();
    virtual ~AdaptiveCyclingAgent() = default;

    void load_model();
    void load_manifest();
    void initialize();

    YAML::Node get_manifest() const { return agent_raw_manifest_; }

    // Called by SQLiteValidationReporter
    void reset_adc_data(int level_id, int vector_size);

    // Replaces finalize_month_580Y_freq() — called from monthly_report_genome_data
    void finalize_month_all_features(
        int level_id,
        int numGenotypes,
        const std::vector<SQLiteValidationReporter::MonthlyGenomeData>&
            monthly_genome_data_by_level
    );

    void inference_from_adc_data(int level_id);

    struct YearMonth { int year, month; };
    static YearMonth   add_months(int y, int m, int offset);
    static std::string ym_to_string(const YearMonth& ym);
};

#endif // ADAPTIVECYCLINGAGENT_H
