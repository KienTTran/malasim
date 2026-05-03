/*
 * SQLiteMonthlyReporter.h
 *
 * Override the base DbReporter and log genotype information at the monthly
 * level.
 */
#ifndef SQLITEMONTHLYREPORTER_H
#define SQLITEMONTHLYREPORTER_H

#include "Reporters/SQLiteDbReporter.h"
#include "Utils/TypeDef.h"

class Person;

class SQLiteMonthlyReporter : public SQLiteDbReporter {
public:
  // disallow copy and move
  SQLiteMonthlyReporter(const SQLiteMonthlyReporter &orig) = delete;
  void operator=(const SQLiteMonthlyReporter &orig) = delete;
  SQLiteMonthlyReporter(SQLiteMonthlyReporter &&orig) = delete;
  void operator=(SQLiteMonthlyReporter &&orig) = delete;

  void monthly_report_genome_data(int monthId) override;
  void monthly_report_site_data(int monthId) override;
  void after_run() override;

protected:
  // Flag to enable cell-level reporting
  bool enable_cell_level_reporting{false};

  struct MonthlySiteData {
    std::vector<double> eir, pfpr_under5, pfpr2to10, pfpr_all;
    std::vector<int> population, clinical_episodes, treatments, treatment_failures, nontreatment,
        treatments_under5, treatments_over5, infections_by_unit;
    std::vector<std::vector<int>> clinical_episodes_by_age_class;
    std::vector<std::vector<int>> clinical_episodes_by_age;
    std::vector<std::vector<int>> population_by_age;
    std::vector<std::vector<double>> total_immune_by_age;
    std::vector<std::vector<int>> multiple_of_infection;
    std::vector<std::vector<int>> number_of_people_seeking_treatment_by_location_age_index;
    std::vector<ul> progress_to_clinical_in_7d_total;
    std::vector<ul> progress_to_clinical_in_7d_recrudescence;
    std::vector<ul> progress_to_clinical_in_7d_new_infection;
    std::vector<ul> recrudescence_treatment;
    std::vector<ul> total_number_of_bites_by_location;
    std::vector<ul> total_number_of_bites_by_location_year;
    std::vector<ul> person_days_by_location_year;
    std::vector<ul> current_foi_by_location;
    std::vector<std::vector<ul>> recrudescence_treatment_by_age_class;
    std::vector<std::vector<ul>> recrudescence_treatment_by_age;

    // 28-day follow-up counters indexed by [unit][outcome] or [unit][outcome][source] etc.
    // outcome: 0=success, 1=failure  source: 0..4
    std::vector<std::vector<ul>> followup_clinical_episodes_28d_total_by_outcome;       // [unit][outcome]
    std::vector<std::vector<ul>> followup_treatments_28d_total_by_outcome;              // [unit][outcome]
    std::vector<std::vector<std::vector<ul>>> followup_clinical_episodes_28d_by_outcome_source; // [unit][outcome][source]
    std::vector<std::vector<std::vector<ul>>> followup_treatments_28d_by_outcome_source;        // [unit][outcome][source]
    std::vector<std::vector<std::vector<ul>>> followup_clinical_episodes_28d_total_by_outcome_age; // [unit][outcome][age]
    std::vector<std::vector<std::vector<ul>>> followup_treatments_28d_total_by_outcome_age;        // [unit][outcome][age]
    std::vector<std::vector<std::vector<ul>>> followup_clinical_episodes_28d_total_by_outcome_age_class; // [unit][outcome][age_class]
    std::vector<std::vector<std::vector<ul>>> followup_treatments_28d_total_by_outcome_age_class;        // [unit][outcome][age_class]
    // [unit][outcome][source][age]
    std::vector<std::vector<std::vector<std::vector<ul>>>> followup_clinical_episodes_28d_by_outcome_source_age;
    std::vector<std::vector<std::vector<std::vector<ul>>>> followup_treatments_28d_by_outcome_source_age;
    // [unit][outcome][source][age_class]
    std::vector<std::vector<std::vector<std::vector<ul>>>> followup_clinical_episodes_28d_by_outcome_source_age_class;
    std::vector<std::vector<std::vector<std::vector<ul>>>> followup_treatments_28d_by_outcome_source_age_class;
  };

  struct MonthlyGenomeData {
    std::vector<std::vector<int>> occurrences;
    std::vector<std::vector<int>> clinical_occurrences;
    std::vector<std::vector<int>> occurrences_0_5;
    std::vector<std::vector<int>> occurrences_2_10;
    std::vector<std::vector<double>> weighted_occurrences;
  };

  std::vector<MonthlySiteData> monthly_site_data_by_level;
  std::vector<MonthlyGenomeData> monthly_genome_data_by_level;

  std::vector<std::string> insert_values;

private:
  void reset_site_data_structures(int level_id, int vector_size, size_t num_age_classes);
  void reset_genome_data_structures(int level_id, int vector_size, size_t num_genotypes);
  void count_infections_for_location(int level_id, int location_id);
  void collect_site_data_for_location(int location, int level_id);
  void calculate_and_build_up_site_data_insert_values(int month_id, int level_id);
  void collect_genome_data_for_location(size_t location, int level_id);
  void collect_genome_data_for_a_person(Person* person, int unit_id, int level_id);
  void build_up_genome_data_insert_values(int month_id, int level_id);

public:
  explicit SQLiteMonthlyReporter(bool cell_level_reporting = false)
      : enable_cell_level_reporting(cell_level_reporting) {}
  ~SQLiteMonthlyReporter() override = default;

  // Initialize the reporter with job number and path
  void initialize(int job_number, const std::string &path) override;

  // Set the flag to enable cell-level reporting
  void set_cell_level_reporting(bool enable) { enable_cell_level_reporting = enable; }
  [[nodiscard]] bool is_cell_level_reporting_enabled() const { return enable_cell_level_reporting; }
};

#endif
