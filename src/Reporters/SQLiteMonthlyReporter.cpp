#include "SQLiteMonthlyReporter.h"

#include <fmt/printf.h>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Population/Population.h"
#include "Simulation//Model.h"
#include "Spatial/GIS/SpatialData.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"

// Initialize the reporter
// Sets up the database and prepares it for data entry
void SQLiteMonthlyReporter::initialize(int job_number, const std::string &path) {
  int admin_level_count = 0;
  if ((Model::get_spatial_data() != nullptr)
      && (Model::get_spatial_data()->get_admin_level_manager() != nullptr)) {
    admin_level_count =
        Model::get_spatial_data()->get_admin_level_manager()->get_level_names().size();
  }
  if (admin_level_count == 0) {
    spdlog::info("No admin levels found, cell level reporting will be enabled.");
    enable_cell_level_reporting = true;
  }

  // Inform the user of the reporter type
  if (enable_cell_level_reporting) {
    if (admin_level_count == 0) {
      spdlog::info("Using SQLiteMonthlyReporter with aggregation at cell/pixel level only.");
    } else {
      spdlog::info(
          "Using SQLiteMonthlyReporter with aggregation at cell/pixel level AND multiple admin "
          "levels.");
    }
  } else {
    spdlog::info(
        "Using SQLiteMonthlyReporter with aggregation at multiple admin levels, cell level "
        "reporting is disabled.");
  }

  SQLiteDbReporter::initialize(job_number, path);

  // Add +1 for cell level
  monthly_site_data_by_level.resize(admin_level_count + 1);
  monthly_genome_data_by_level.resize(admin_level_count + 1);
  CELL_LEVEL_ID = admin_level_count;
}

void SQLiteMonthlyReporter::count_infections_for_location(int level_id, int location_id) {
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);
  std::vector<int> age_classes = Model::get_config()->age_structure();
  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    for (unsigned int ac = 0; ac < age_classes.size(); ac++) {
      for (auto &person : index->vPerson()[location_id][hs][ac]) {
        // Is the individual infected by at least one parasite?
        if (person->get_all_clonal_parasite_populations()->empty()) { continue; }

        monthly_site_data_by_level[level_id].infections_by_unit[unit_id]++;
      }
    }
  }
}

void SQLiteMonthlyReporter::calculate_and_build_up_site_data_insert_values(int month_id,
                                                                           int level_id) {
  int min_unit_id = 0;
  int max_unit_id = 0;
  if (level_id == CELL_LEVEL_ID) {
    min_unit_id = 0;
    max_unit_id = Model::get_config()->number_of_locations() - 1;
  } else {
    // Get the boundary for this admin level
    const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
        Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);

    min_unit_id = boundary->min_unit_id;
    max_unit_id = boundary->max_unit_id;
  }

  insert_values.clear();

  for (auto unit_id = min_unit_id; unit_id <= max_unit_id; unit_id++) {
    // Skip units with no population
    if (monthly_site_data_by_level[level_id].population[unit_id] == 0) continue;

    double calculated_eir = (monthly_site_data_by_level[level_id].eir[unit_id] != 0)
                                ? (monthly_site_data_by_level[level_id].eir[unit_id]
                                   / monthly_site_data_by_level[level_id].population[unit_id])
                                : 0;
    double calculated_pfpr_under5 =
        (monthly_site_data_by_level[level_id].pfpr_under5[unit_id] != 0)
            ? (monthly_site_data_by_level[level_id].pfpr_under5[unit_id]
               / monthly_site_data_by_level[level_id].population[unit_id])
                  * 100.0
            : 0;
    double calculated_pfpr2to10 = (monthly_site_data_by_level[level_id].pfpr2to10[unit_id] != 0)
                                      ? (monthly_site_data_by_level[level_id].pfpr2to10[unit_id]
                                         / monthly_site_data_by_level[level_id].population[unit_id])
                                            * 100.0
                                      : 0;
    double calculated_pfpr_all = (monthly_site_data_by_level[level_id].pfpr_all[unit_id] != 0)
                                     ? (monthly_site_data_by_level[level_id].pfpr_all[unit_id]
                                        / monthly_site_data_by_level[level_id].population[unit_id])
                                           * 100.0
                                     : 0;

    std::string single_row =
        fmt::format("({}, {}, {}, {}", month_id, unit_id,
                    monthly_site_data_by_level[level_id].population[unit_id],
                    monthly_site_data_by_level[level_id].clinical_episodes[unit_id]);

    // Append clinical episodes by age class
    for (const auto &episodes :
         monthly_site_data_by_level[level_id].clinical_episodes_by_age_class[unit_id]) {
      single_row += fmt::format(", {}", episodes);
    }

    for (const auto &treatment :
         monthly_site_data_by_level[level_id].recrudescence_treatment_by_age_class[unit_id]) {
      single_row += fmt::format(", {}", treatment);
    }

    for (const auto &episodes :
         monthly_site_data_by_level[level_id].clinical_episodes_by_age[unit_id]) {
      single_row += fmt::format(", {}", episodes);
    }

    for (const auto &population : monthly_site_data_by_level[level_id].population_by_age[unit_id]) {
      single_row += fmt::format(", {}", population);
    }

    for (const auto &immune : monthly_site_data_by_level[level_id].total_immune_by_age[unit_id]) {
      single_row += fmt::format(", {}", immune);
    }

    for (const auto &treatment :
         monthly_site_data_by_level[level_id].recrudescence_treatment_by_age[unit_id]) {
      single_row += fmt::format(", {}", treatment);
    }

    for (const auto &moi : monthly_site_data_by_level[level_id].multiple_of_infection[unit_id]) {
      single_row += fmt::format(", {}", moi);
    }

    // Append age-indexed seeking-treatment counts (if present)
    for (const auto &count :
         monthly_site_data_by_level[level_id]
             .number_of_people_seeking_treatment_by_location_age_index[unit_id]) {
      single_row += fmt::format(", {}", count);
    }

    single_row += fmt::format(
        ", {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
        monthly_site_data_by_level[level_id].treatments[unit_id], calculated_eir,
        calculated_pfpr_under5, calculated_pfpr2to10, calculated_pfpr_all,
        monthly_site_data_by_level[level_id].infections_by_unit[unit_id],
        monthly_site_data_by_level[level_id].treatment_failures[unit_id],
        monthly_site_data_by_level[level_id].nontreatment[unit_id],
        monthly_site_data_by_level[level_id].treatments_under5[unit_id],
        monthly_site_data_by_level[level_id].treatments_over5[unit_id],
        monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_total[unit_id],
        monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_recrudescence[unit_id],
        monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_new_infection[unit_id],
        monthly_site_data_by_level[level_id].recrudescence_treatment[unit_id],
        monthly_site_data_by_level[level_id].total_number_of_bites_by_location[unit_id],
        monthly_site_data_by_level[level_id].total_number_of_bites_by_location_year[unit_id],
        monthly_site_data_by_level[level_id].person_days_by_location_year[unit_id],
        monthly_site_data_by_level[level_id].current_foi_by_location[unit_id]);

    // Append 28-day follow-up columns
    const int n_outcomes = ModelDataCollector::FOLLOWUP_OUTCOMES;
    const int n_sources = ModelDataCollector::FOLLOWUP_SOURCES;
    for (int o = 0; o < n_outcomes; o++) {
      single_row += fmt::format(", {}", monthly_site_data_by_level[level_id]
                                            .followup_clinical_episodes_28d_total_by_outcome[unit_id][o]);
      for (int s = 0; s < n_sources; s++) {
        single_row += fmt::format(", {}", monthly_site_data_by_level[level_id]
                                              .followup_clinical_episodes_28d_by_outcome_source[unit_id][o][s]);
      }
      // source_sum column (sum of 5 sources – redundant but kept for SQL debugging)
      ul src_sum_ce = 0;
      for (int s = 0; s < n_sources; s++) {
        src_sum_ce += monthly_site_data_by_level[level_id]
                          .followup_clinical_episodes_28d_by_outcome_source[unit_id][o][s];
      }
      single_row += fmt::format(", {}", src_sum_ce);

      single_row += fmt::format(", {}", monthly_site_data_by_level[level_id]
                                            .followup_treatments_28d_total_by_outcome[unit_id][o]);
      for (int s = 0; s < n_sources; s++) {
        single_row += fmt::format(", {}", monthly_site_data_by_level[level_id]
                                              .followup_treatments_28d_by_outcome_source[unit_id][o][s]);
      }
      ul src_sum_tx = 0;
      for (int s = 0; s < n_sources; s++) {
        src_sum_tx += monthly_site_data_by_level[level_id]
                          .followup_treatments_28d_by_outcome_source[unit_id][o][s];
      }
      single_row += fmt::format(", {}", src_sum_tx);
    }
    single_row += ")";
    insert_values.push_back(single_row);
  }
}

// Collect and store monthly site data
// Aggregates data related to various site metrics and stores them in the
// database
void SQLiteMonthlyReporter::monthly_report_site_data(int month_id) {
  TransactionGuard transaction{db.get()};

  // Handle all levels including cell level in a single loop
  int total_levels = monthly_site_data_by_level.size();

  for (int level_id = 0; level_id < total_levels; level_id++) {
    // level_id == -1 represents cell level
    bool is_cell_level = (level_id == CELL_LEVEL_ID);

    // Skip cell level if not enabled
    if (is_cell_level && !enable_cell_level_reporting) continue;

    int vector_size = -1;
    // Get vector size based on level
    if (is_cell_level) {
      vector_size = Model::get_config()->number_of_locations();
    } else {
      const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
          Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);
      vector_size = boundary->max_unit_id + 1;
    }

    std::vector<int> age_classes = Model::get_config()->age_structure();
    // Reset data structures for this admin level
    reset_site_data_structures(level_id, vector_size, age_classes.size());

    // Collect data for this admin level
    for (auto location = 0; location < Model::get_config()->number_of_locations(); location++) {
      auto location_population = static_cast<int>(Model::get_population()->size(location));
      if (location_population == 0) continue;

      collect_site_data_for_location(location, level_id);
    }

    // Calculate and insert data for this admin level
    calculate_and_build_up_site_data_insert_values(month_id, level_id);
    insert_monthly_site_data(level_id, insert_values);
  }
}

void SQLiteMonthlyReporter::collect_site_data_for_location(int location_id, int level_id) {
  // Get admin unit for this location at this level
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);

  std::vector<int> age_classes = Model::get_config()->age_structure();

  count_infections_for_location(level_id, location_id);

  auto location_population = static_cast<int>(Model::get_population()->size(location_id));
  // Collect the simple data
  monthly_site_data_by_level[level_id].population[unit_id] += location_population;

  monthly_site_data_by_level[level_id].clinical_episodes[unit_id] +=
      Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[location_id];

  monthly_site_data_by_level[level_id].treatments[unit_id] +=
      Model::get_mdc()->monthly_number_of_treatment_by_location()[location_id];
  monthly_site_data_by_level[level_id].treatment_failures[unit_id] +=
      Model::get_mdc()->monthly_treatment_failure_by_location()[location_id];
  monthly_site_data_by_level[level_id].nontreatment[unit_id] +=
      Model::get_mdc()->monthly_nontreatment_by_location()[location_id];

  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_total[unit_id] +=
      Model::get_mdc()->progress_to_clinical_in_7d_counter[location_id].total;

  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_recrudescence[unit_id] +=
      Model::get_mdc()->progress_to_clinical_in_7d_counter[location_id].recrudescence;

  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_new_infection[unit_id] +=
      Model::get_mdc()->progress_to_clinical_in_7d_counter[location_id].new_infection;

  monthly_site_data_by_level[level_id].recrudescence_treatment[unit_id] +=
      Model::get_mdc()->monthly_number_of_recrudescence_treatment_by_location()[location_id];

  for (auto ndx = 0; ndx < age_classes.size(); ndx++) {
    // Collect the treatment by age class, following the 0-59 month convention
    // for under-5
    if (age_classes[ndx] < 5) {
      monthly_site_data_by_level[level_id].treatments_under5[unit_id] +=
          Model::get_mdc()->monthly_number_of_treatment_by_location_age_class()[location_id][ndx];
    } else {
      monthly_site_data_by_level[level_id].treatments_over5[unit_id] +=
          Model::get_mdc()->monthly_number_of_treatment_by_location_age_class()[location_id][ndx];
    }

    // collect the clinical episodes by age class
    monthly_site_data_by_level[level_id].clinical_episodes_by_age_class[unit_id][ndx] +=
        Model::get_mdc()
            ->monthly_number_of_clinical_episode_by_location_age_class()[location_id][ndx];
  }

  for (auto age = 0; age < 80; age++) {
    monthly_site_data_by_level[level_id].clinical_episodes_by_age[unit_id][age] +=
        Model::get_mdc()->monthly_number_of_clinical_episode_by_location_age()[location_id][age];
  }

  for (auto age = 0; age < 80; age++) {
    monthly_site_data_by_level[level_id].population_by_age[unit_id][age] +=
        Model::get_mdc()->popsize_by_location_age()[location_id][age];
  }

  for (auto age = 0; age < 80; age++) {
    monthly_site_data_by_level[level_id].total_immune_by_age[unit_id][age] +=
        Model::get_mdc()->total_immune_by_location_age()[location_id][age];
  }

  for (auto ndx = 0; ndx < age_classes.size(); ndx++) {
    monthly_site_data_by_level[level_id].recrudescence_treatment_by_age_class[unit_id][ndx] +=
        Model::get_mdc()
            ->monthly_number_of_recrudescence_treatment_by_location_age_class()[location_id][ndx];
  }

  for (auto age = 0; age < 80; age++) {
    monthly_site_data_by_level[level_id].recrudescence_treatment_by_age[unit_id][age] +=
        Model::get_mdc()
            ->monthly_number_of_recrudescence_treatment_by_location_age()[location_id][age];
  }

  for (auto moi = 0; moi < ModelDataCollector::NUMBER_OF_REPORTED_MOI; moi++) {
    monthly_site_data_by_level[level_id].multiple_of_infection[unit_id][moi] +=
        Model::get_mdc()->multiple_of_infection_by_location()[location_id][moi];
  }

  // EIR and PfPR is a bit more complicated since it could be an invalid value
  // early in the simulation, and when aggregating at the district level the
  // weighted mean needs to be reported instead
  if (Model::get_mdc()->recording_data()) {
    monthly_site_data_by_level[level_id].total_number_of_bites_by_location_year[unit_id] +=
        Model::get_mdc()->total_number_of_bites_by_location_year()[location_id];

    monthly_site_data_by_level[level_id].total_number_of_bites_by_location[unit_id] +=
        Model::get_mdc()->total_number_of_bites_by_location()[location_id];

    monthly_site_data_by_level[level_id].person_days_by_location_year[unit_id] +=
        Model::get_mdc()->person_days_by_location_year()[location_id];

    monthly_site_data_by_level[level_id].current_foi_by_location[unit_id] +=
        Model::get_population()->current_force_of_infection_by_location()[location_id];

    auto eir_location = Model::get_mdc()->eir_by_location_year()[location_id].empty()
                            ? 0
                            : Model::get_mdc()->eir_by_location_year()[location_id].back();
    monthly_site_data_by_level[level_id].eir[unit_id] += (eir_location * location_population);
    monthly_site_data_by_level[level_id].pfpr_under5[unit_id] +=
        (Model::get_mdc()->get_blood_slide_prevalence(location_id, 0, 5) * location_population);
    monthly_site_data_by_level[level_id].pfpr2to10[unit_id] +=
        (Model::get_mdc()->get_blood_slide_prevalence(location_id, 2, 10) * location_population);
    monthly_site_data_by_level[level_id].pfpr_all[unit_id] +=
        (Model::get_mdc()->blood_slide_prevalence_by_location()[location_id] * location_population);
  }

  const auto &mdc_age_index =
      Model::get_mdc()->monthly_number_of_people_seeking_treatment_by_location_age_index();
  if (!mdc_age_index.empty() && location_id < static_cast<int>(mdc_age_index.size())) {
    const auto &vec = mdc_age_index[location_id];
    // Ensure target vector is large enough
    for (size_t idx = 0;
         idx < vec.size()
         && idx < monthly_site_data_by_level[level_id]
                      .number_of_people_seeking_treatment_by_location_age_index[unit_id]
                      .size();
         ++idx) {
      monthly_site_data_by_level[level_id]
          .number_of_people_seeking_treatment_by_location_age_index[unit_id][idx] += vec[idx];
    }
  }

  // Aggregate 28-day follow-up counters
  const int n_outcomes = ModelDataCollector::FOLLOWUP_OUTCOMES;
  const int n_sources = ModelDataCollector::FOLLOWUP_SOURCES;
  const int n_age_bins = ModelDataCollector::FOLLOWUP_AGE_BINS;
  const auto &mdc_fu_ce = Model::get_mdc()->followup_clinical_episodes_28d_by_location_outcome();
  const auto &mdc_fu_ce_src = Model::get_mdc()->followup_clinical_episodes_28d_by_location_outcome_source();
  const auto &mdc_fu_ce_age = Model::get_mdc()->followup_clinical_episodes_28d_by_location_outcome_age();
  const auto &mdc_fu_ce_src_age = Model::get_mdc()->followup_clinical_episodes_28d_by_location_outcome_source_age();
  const auto &mdc_fu_ce_ac = Model::get_mdc()->followup_clinical_episodes_28d_by_location_outcome_age_class();
  const auto &mdc_fu_ce_src_ac = Model::get_mdc()->followup_clinical_episodes_28d_by_location_outcome_source_age_class();

  const auto &mdc_fu_tx = Model::get_mdc()->followup_treatments_28d_by_location_outcome();
  const auto &mdc_fu_tx_src = Model::get_mdc()->followup_treatments_28d_by_location_outcome_source();
  const auto &mdc_fu_tx_age = Model::get_mdc()->followup_treatments_28d_by_location_outcome_age();
  const auto &mdc_fu_tx_src_age = Model::get_mdc()->followup_treatments_28d_by_location_outcome_source_age();
  const auto &mdc_fu_tx_ac = Model::get_mdc()->followup_treatments_28d_by_location_outcome_age_class();
  const auto &mdc_fu_tx_src_ac = Model::get_mdc()->followup_treatments_28d_by_location_outcome_source_age_class();

  for (int o = 0; o < n_outcomes; o++) {
    monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_total_by_outcome[unit_id][o] +=
        mdc_fu_ce[location_id][o];
    monthly_site_data_by_level[level_id].followup_treatments_28d_total_by_outcome[unit_id][o] +=
        mdc_fu_tx[location_id][o];
    for (int s = 0; s < n_sources; s++) {
      monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_by_outcome_source[unit_id][o][s] +=
          mdc_fu_ce_src[location_id][o][s];
      monthly_site_data_by_level[level_id].followup_treatments_28d_by_outcome_source[unit_id][o][s] +=
          mdc_fu_tx_src[location_id][o][s];
    }
    for (int a = 0; a < n_age_bins; a++) {
      monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_total_by_outcome_age[unit_id][o][a] +=
          mdc_fu_ce_age[location_id][o][a];
      monthly_site_data_by_level[level_id].followup_treatments_28d_total_by_outcome_age[unit_id][o][a] +=
          mdc_fu_tx_age[location_id][o][a];
    }
    for (int ac2 = 0; ac2 < static_cast<int>(age_classes.size()); ac2++) {
      monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_total_by_outcome_age_class[unit_id][o][ac2] +=
          mdc_fu_ce_ac[location_id][o][ac2];
      monthly_site_data_by_level[level_id].followup_treatments_28d_total_by_outcome_age_class[unit_id][o][ac2] +=
          mdc_fu_tx_ac[location_id][o][ac2];
    }
    for (int s = 0; s < n_sources; s++) {
      for (int a = 0; a < n_age_bins; a++) {
        monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_by_outcome_source_age[unit_id][o][s][a] +=
            mdc_fu_ce_src_age[s][location_id][o][a];
        monthly_site_data_by_level[level_id].followup_treatments_28d_by_outcome_source_age[unit_id][o][s][a] +=
            mdc_fu_tx_src_age[s][location_id][o][a];
      }
      for (int ac2 = 0; ac2 < static_cast<int>(age_classes.size()); ac2++) {
        monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_by_outcome_source_age_class[unit_id][o][s][ac2] +=
            mdc_fu_ce_src_ac[s][location_id][o][ac2];
        monthly_site_data_by_level[level_id].followup_treatments_28d_by_outcome_source_age_class[unit_id][o][s][ac2] +=
            mdc_fu_tx_src_ac[s][location_id][o][ac2];
      }
    }
  }
}

void SQLiteMonthlyReporter::collect_genome_data_for_location(size_t location_id, int level_id) {
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);
  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto age_classes = index->vPerson()[0][0].size();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    // Iterate over all the age classes
    for (unsigned int ac = 0; ac < age_classes; ac++) {
      // Iterate over all the genotypes
      auto people_in_age_class = index->vPerson()[location_id][hs][ac];
      for (auto &person : people_in_age_class) {
        collect_genome_data_for_a_person(person, unit_id, level_id);
      }
    }
  }
}

void SQLiteMonthlyReporter::reset_site_data_structures(int level_id, int vector_size,
                                                       size_t num_age_classes) {
  // reset the data structures
  monthly_site_data_by_level[level_id].eir.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr_under5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr2to10.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr_all.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].population.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].clinical_episodes.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].clinical_episodes_by_age_class.assign(
      vector_size, std::vector<int>(num_age_classes, 0));
  monthly_site_data_by_level[level_id].clinical_episodes_by_age.assign(vector_size,
                                                                       std::vector<int>(80, 0));
  monthly_site_data_by_level[level_id].population_by_age.assign(vector_size,
                                                                std::vector<int>(80, 0));
  monthly_site_data_by_level[level_id].total_immune_by_age.assign(vector_size,
                                                                  std::vector<double>(80, 0));
  monthly_site_data_by_level[level_id].recrudescence_treatment_by_age_class.assign(
      vector_size, std::vector<ul>(num_age_classes, 0));
  monthly_site_data_by_level[level_id].recrudescence_treatment_by_age.assign(
      vector_size, std::vector<ul>(80, 0));
  monthly_site_data_by_level[level_id].multiple_of_infection.assign(
      vector_size, std::vector<int>(ModelDataCollector::NUMBER_OF_REPORTED_MOI, 0));
  const auto age_index_count =
      static_cast<int>(Model::get_config()
                           ->get_epidemiological_parameters()
                           .get_age_based_probability_of_seeking_treatment()
                           .get_ages()
                           .size());
  monthly_site_data_by_level[level_id]
      .number_of_people_seeking_treatment_by_location_age_index.assign(
          vector_size, std::vector<int>((age_index_count > 0) ? age_index_count : 1, 0));
  monthly_site_data_by_level[level_id].treatments.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatment_failures.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].nontreatment.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatments_under5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatments_over5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].infections_by_unit.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_total.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_recrudescence.assign(vector_size,
                                                                                       0);
  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_new_infection.assign(vector_size,
                                                                                       0);
  monthly_site_data_by_level[level_id].recrudescence_treatment.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].total_number_of_bites_by_location.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].total_number_of_bites_by_location_year.assign(vector_size,
                                                                                     0);
  monthly_site_data_by_level[level_id].person_days_by_location_year.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].current_foi_by_location.assign(vector_size, 0);

  // Reset 28-day follow-up fields
  const int n_outcomes = ModelDataCollector::FOLLOWUP_OUTCOMES;
  const int n_sources = ModelDataCollector::FOLLOWUP_SOURCES;
  const int n_age_bins = ModelDataCollector::FOLLOWUP_AGE_BINS;

  monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_total_by_outcome.assign(
      vector_size, std::vector<ul>(n_outcomes, 0));
  monthly_site_data_by_level[level_id].followup_treatments_28d_total_by_outcome.assign(
      vector_size, std::vector<ul>(n_outcomes, 0));
  monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_by_outcome_source.assign(
      vector_size, std::vector<std::vector<ul>>(n_outcomes, std::vector<ul>(n_sources, 0)));
  monthly_site_data_by_level[level_id].followup_treatments_28d_by_outcome_source.assign(
      vector_size, std::vector<std::vector<ul>>(n_outcomes, std::vector<ul>(n_sources, 0)));
  monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_total_by_outcome_age.assign(
      vector_size, std::vector<std::vector<ul>>(n_outcomes, std::vector<ul>(n_age_bins, 0)));
  monthly_site_data_by_level[level_id].followup_treatments_28d_total_by_outcome_age.assign(
      vector_size, std::vector<std::vector<ul>>(n_outcomes, std::vector<ul>(n_age_bins, 0)));
  monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_total_by_outcome_age_class.assign(
      vector_size, std::vector<std::vector<ul>>(n_outcomes, std::vector<ul>(num_age_classes, 0)));
  monthly_site_data_by_level[level_id].followup_treatments_28d_total_by_outcome_age_class.assign(
      vector_size, std::vector<std::vector<ul>>(n_outcomes, std::vector<ul>(num_age_classes, 0)));
  monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_by_outcome_source_age.assign(
      vector_size, std::vector<std::vector<std::vector<ul>>>(
          n_outcomes, std::vector<std::vector<ul>>(n_sources, std::vector<ul>(n_age_bins, 0))));
  monthly_site_data_by_level[level_id].followup_treatments_28d_by_outcome_source_age.assign(
      vector_size, std::vector<std::vector<std::vector<ul>>>(
          n_outcomes, std::vector<std::vector<ul>>(n_sources, std::vector<ul>(n_age_bins, 0))));
  monthly_site_data_by_level[level_id].followup_clinical_episodes_28d_by_outcome_source_age_class.assign(
      vector_size, std::vector<std::vector<std::vector<ul>>>(
          n_outcomes, std::vector<std::vector<ul>>(n_sources, std::vector<ul>(num_age_classes, 0))));
  monthly_site_data_by_level[level_id].followup_treatments_28d_by_outcome_source_age_class.assign(
      vector_size, std::vector<std::vector<std::vector<ul>>>(
          n_outcomes, std::vector<std::vector<ul>>(n_sources, std::vector<ul>(num_age_classes, 0))));
}

void SQLiteMonthlyReporter::reset_genome_data_structures(int level_id, int vector_size,
                                                         size_t num_genotypes) {
  // reset the data structures
  monthly_genome_data_by_level[level_id].occurrences.assign(vector_size,
                                                            std::vector<int>(num_genotypes, 0));
  monthly_genome_data_by_level[level_id].clinical_occurrences.assign(
      vector_size, std::vector<int>(num_genotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_0_5.assign(vector_size,
                                                                std::vector<int>(num_genotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_2_10.assign(
      vector_size, std::vector<int>(num_genotypes, 0));
  monthly_genome_data_by_level[level_id].weighted_occurrences.assign(
      vector_size, std::vector<double>(num_genotypes, 0));
}

void SQLiteMonthlyReporter::collect_genome_data_for_a_person(Person* person, int unit_id,
                                                             int level_id) {
  const auto num_genotypes = Config::number_of_parasite_types();
  auto individual = std::vector<int>(num_genotypes, 0);
  // Get the person, press on if they are not infected
  auto &parasites = *person->get_all_clonal_parasite_populations();
  auto num_clones = parasites.size();
  if (num_clones == 0) { return; }

  // Note the age and clinical status of the person
  auto age = person->get_age();
  auto clinical = static_cast<int>(person->get_host_state() == Person::HostStates::CLINICAL);

  // Count the genotypes present in the individual
  for (unsigned int ndx = 0; ndx < num_clones; ndx++) {
    auto* parasite_population = parasites[ndx];
    auto genotype_id = parasite_population->genotype()->genotype_id();
    monthly_genome_data_by_level[level_id].occurrences[unit_id][genotype_id]++;
    monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotype_id] +=
        (age <= 5) ? 1 : 0;
    monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotype_id] +=
        (age >= 2 && age <= 10) ? 1 : 0;
    individual[genotype_id]++;

    // Count a clinical occurrence if the individual has clinical
    // symptoms
    monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotype_id] += clinical;
  }

  // Update the weighted occurrences and reset the individual count
  for (unsigned int ndx = 0; ndx < num_genotypes; ndx++) {
    if (individual[ndx] == 0) { continue; }
    monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][ndx] +=
        (individual[ndx] / static_cast<double>(num_clones));
  }
}

void SQLiteMonthlyReporter::build_up_genome_data_insert_values(int month_id, int level_id) {
  auto num_genotypes = Config::number_of_parasite_types();

  int min_unit_id = 0;
  int max_unit_id = 0;
  if (level_id == CELL_LEVEL_ID) {
    min_unit_id = 0;
    max_unit_id = Model::get_config()->number_of_locations() - 1;
  } else {
    // Get the boundary for this admin level
    const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
        Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);
    min_unit_id = boundary->min_unit_id;
    max_unit_id = boundary->max_unit_id;
  }

  insert_values.clear();

  // Iterate over the admin units and append the query
  for (auto unit_id = min_unit_id; unit_id <= max_unit_id; unit_id++) {
    // Skip if there are no infections in this unit
    if (monthly_site_data_by_level[level_id].infections_by_unit[unit_id] == 0) { continue; }

    for (auto genotype = 0; genotype < num_genotypes; genotype++) {
      if (monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype] == 0) {
        continue;
      }
      std::string single_row = fmt::format(
          "({}, {}, {}, {}, {}, {}, {}, {})", month_id, unit_id, genotype,
          monthly_genome_data_by_level[level_id].occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotype],
          monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype]);

      insert_values.push_back(single_row);
    }
  }
}

void SQLiteMonthlyReporter::monthly_report_genome_data(int month_id) {
  TransactionGuard transaction{db.get()};

  // Get admin levels count
  int admin_level_count = monthly_site_data_by_level.size();

  // For each admin level
  for (int level_id = 0; level_id < admin_level_count; level_id++) {
    // level_id == -1 represents cell level
    bool is_cell_level = (level_id == CELL_LEVEL_ID);

    // Skip cell level if not enabled
    if (is_cell_level && !enable_cell_level_reporting) continue;

    int vector_size = -1;
    // Get vector size based on level
    if (is_cell_level) {
      vector_size = Model::get_config()->number_of_locations();
    } else {
      const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
          Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);
      vector_size = boundary->max_unit_id + 1;
    }

    auto num_genotypes = Config::number_of_parasite_types();

    reset_genome_data_structures(level_id, vector_size, num_genotypes);

    auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

    // Iterate over all locations
    for (auto location = 0; location < index->vPerson().size(); location++) {
      collect_genome_data_for_location(location, level_id);
    }

    build_up_genome_data_insert_values(month_id, level_id);

    if (insert_values.empty()) {
      spdlog::info("No genotypes recorded in the simulation at timestep, {}",
                   Model::get_scheduler()->current_time());
      continue;
    }

    insert_monthly_genome_data(level_id, insert_values);
  }
}

void SQLiteMonthlyReporter::after_run() { SQLiteDbReporter::after_run(); }
