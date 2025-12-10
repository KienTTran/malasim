#include "SQLiteValidationReporter.h"

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
void SQLiteValidationReporter::initialize(int jobNumber, const std::string &path) {
  spdlog::info("SQLiteValidationReporter initialize");
  int admin_level_count =
      Model::get_spatial_data()->get_admin_level_manager()->get_level_names().size();

  if (admin_level_count == 0) {
    spdlog::info("No admin levels found, cell level reporting will be enabled.");
    enable_cell_level_reporting = true;
  }

  // Inform the user of the reporter type
  if (enable_cell_level_reporting) {
    spdlog::info(
        "Using SQLiteValidationReporter with aggregation at cell/pixel level AND multiple admin "
        "levels.");
  } else {
    spdlog::info(
        "Using SQLiteValidationReporter with aggregation at multiple admin levels, cell level "
        "reporting is disabled.");
  }

  SQLiteDbReporter::initialize(jobNumber, path + "validation_");

  // Add +1 for cell level
  monthly_site_data_by_level.resize(admin_level_count + 1);
  monthly_genome_data_by_level.resize(admin_level_count + 1);

    // Include cell level in the number of levels
  insert_site_query_prefixes_.resize(admin_level_count + 1);
  insert_genome_query_prefixes_.resize(admin_level_count + 1);

  CELL_LEVEL_ID = admin_level_count;
}

std::string SQLiteValidationReporter::get_site_table_name(int level_id) const {
  if (level_id == CELL_LEVEL_ID) { return "monthly_site_data_cell"; }
  return "monthly_site_data_" + Model::get_spatial_data()->get_admin_level_name(level_id);
}

std::string SQLiteValidationReporter::get_genome_table_name(int level_id) const {
  if (level_id == CELL_LEVEL_ID) { return "monthly_genome_data_cell"; }
  return "monthly_genome_data_" + Model::get_spatial_data()->get_admin_level_name(level_id);
}

// Function to create tables for each admin level
void SQLiteValidationReporter::create_all_reporting_tables() {
  auto admin_levels = Model::get_spatial_data()->get_admin_level_manager()->get_level_names();

  std::string age_class_column_definitions;
  for (auto ndx = 0; ndx < Model::get_config()->age_structure().size(); ndx++) {
    auto ag_from = ndx == 0 ? 0 : Model::get_config()->age_structure()[ndx - 1];
    auto ag_to = Model::get_config()->age_structure()[ndx];
    age_class_column_definitions +=
        fmt::format("clinical_episodes_by_age_class_{}_{} INTEGER, ", ag_from, ag_to);
  }

  for (auto ndx = 0; ndx < Model::get_config()->age_structure().size(); ndx++) {
    auto ag_from = ndx == 0 ? 0 : Model::get_config()->age_structure()[ndx - 1];
    auto ag_to = Model::get_config()->age_structure()[ndx];
    age_class_column_definitions +=
        fmt::format("recrudescence_treatment_by_age_class_{}_{} INTEGER, ", ag_from, ag_to);
  }

  std::string age_class_columns;
  for (auto ndx = 0; ndx < Model::get_config()->age_structure().size(); ndx++) {
    auto ag_from = ndx == 0 ? 0 : Model::get_config()->age_structure()[ndx - 1];
    auto ag_to = Model::get_config()->age_structure()[ndx];
    age_class_columns += fmt::format("clinical_episodes_by_age_class_{}_{}, ", ag_from, ag_to);
  }

  for (auto ndx = 0; ndx < Model::get_config()->age_structure().size(); ndx++) {
    auto ag_from = ndx == 0 ? 0 : Model::get_config()->age_structure()[ndx - 1];
    auto ag_to = Model::get_config()->age_structure()[ndx];
    age_class_columns += fmt::format("recrudescence_treatment_by_age_class_{}_{}, ", ag_from, ag_to);
  }

  std::string age_column_definitions;
  for (auto age = 0; age < 80; age++) {
    age_column_definitions +=
        fmt::format("clinical_episodes_by_age_{} INTEGER, ", age);
  }

  for (auto age = 0; age < 80; age++) {
    age_column_definitions +=
        fmt::format("population_by_age_{} INTEGER, ", age);
  }

  for (auto age = 0; age < 80; age++) {
    age_column_definitions +=
        fmt::format("total_immune_by_age_{} REAL, ", age);
  }

  for (auto age = 0; age < 80; age++) {
    age_column_definitions +=
        fmt::format("recrudescence_treatment_by_age_{} INTEGER, ", age);
  }

  for (auto moi = 0; moi < ModelDataCollector::NUMBER_OF_REPORTED_MOI; moi++) {
    age_column_definitions += fmt::format("moi_{} INTEGER, ", moi);
  }

  std::string age_columns;
  for (auto age = 0; age < 80; age++) {
    age_columns += fmt::format("clinical_episodes_by_age_{}, ", age);
  }
  for (auto age = 0; age < 80; age++) {
    age_columns += fmt::format("population_by_age_{}, ", age);
  }
  for (auto age = 0; age < 80; age++) {
    age_columns += fmt::format("total_immune_by_age_{}, ", age);
  }
  for (auto age = 0; age < 80; age++) {
    age_columns += fmt::format("recrudescence_treatment_by_age_{}, ", age);
  }
  for (auto moi = 0; moi < ModelDataCollector::NUMBER_OF_REPORTED_MOI; moi++) {
    age_columns += fmt::format("moi_{}, ", moi);
  }

  // // Include cell level in the number of levels
  // int number_of_levels = admin_levels.size() + 1;

  // // Resize the query prefix vectors to include all admin levels plus cell level
  // insert_site_query_prefixes_.resize(number_of_levels);
  // insert_genome_query_prefixes_.resize(number_of_levels);

  // Now create tables for each admin level including cell level
  for (size_t level_id = 0; level_id < admin_levels.size() + 1; level_id++) {
    create_reporting_tables_for_level(level_id,
      age_class_column_definitions, age_class_columns,
      age_column_definitions,
      age_columns);
  }
}

void SQLiteValidationReporter::create_reporting_tables_for_level(
    int level_id, const std::string &age_class_column_definitions,
    const std::string &age_class_columns,
    const std::string &age_column_definitions,
    const std::string &age_columns) {
  // Generate table names for this level
  std::string site_table_name = get_site_table_name(level_id);
  std::string genome_table_name = get_genome_table_name(level_id);

  // location_id for cell level or unit_id for admin level
  std::string location_id_column = (level_id == CELL_LEVEL_ID) ? "location_id" : "unit_id";

  // Create site data table for this level
  std::string create_site_data_table = fmt::format(R""""(
      CREATE TABLE IF NOT EXISTS {} (
          monthly_data_id INTEGER NOT NULL,
          {} INTEGER NOT NULL,
          population INTEGER NOT NULL,
          clinical_episodes INTEGER NOT NULL, )"""",
                                                site_table_name, location_id_column)
                                    + age_class_column_definitions
                                    + age_column_definitions
                                    + fmt::format(R""""(
          treatments INTEGER NOT NULL,
          treatment_failures INTEGER NOT NULL,
          eir REAL NOT NULL,
          pfpr_under5 REAL NOT NULL,
          pfpr_2to10 REAL NOT NULL,
          pfpr_all REAL NOT NULL,
          infected_individuals INTEGER,
          non_treatment INTEGER NOT NULL,
          under5_treatment INTEGER NOT NULL,
          over5_treatment INTEGER NOT NULL,
          progress_to_clinical_in_7d_total BIGINT NOT NULL,
          progress_to_clinical_in_7d_recrudescence BIGINT NOT NULL,
          progress_to_clinical_in_7d_new_infection BIGINT NOT NULL,
          recrudescence_treatment BIGINT NOT NULL,
          total_number_of_bites_by_location BIGINT NOT NULL,
          total_number_of_bites_by_location_year BIGINT NOT NULL,
          person_days_by_location_year BIGINT NOT NULL,
          current_foi_by_location BIGINT NOT NULL,
          PRIMARY KEY (monthly_data_id, {}),
          FOREIGN KEY (monthly_data_id) REFERENCES monthly_data(id)
      );
    )"""",
                                                  location_id_column);

  // Create genome data table for this level
  std::string create_genome_data_table =
      fmt::format(R""""(
      CREATE TABLE IF NOT EXISTS {} (
          monthly_data_id INTEGER NOT NULL,
          {} INTEGER NOT NULL,
          genome_id INTEGER NOT NULL,
          occurrences INTEGER NOT NULL,
          clinical_occurrences INTEGER NOT NULL,
          occurrences_0to5 INTEGER NOT NULL,
          occurrences_2to10 INTEGER NOT NULL,
          weighted_occurrences REAL NOT NULL,
          PRIMARY KEY (monthly_data_id, genome_id, {}),
          FOREIGN KEY (genome_id) REFERENCES genotype(id),
          FOREIGN KEY (monthly_data_id) REFERENCES monthly_data(id)
      );
    )"""",
                  genome_table_name, location_id_column, location_id_column);
  try {
    // Execute the creation queries
    db->execute(create_site_data_table);
    db->execute(create_genome_data_table);

    // Determine index for query prefixes
    int prefix_index =
        (level_id == CELL_LEVEL_ID) ? insert_site_query_prefixes_.size() - 1 : level_id;

    // Create insert query prefixes for this level
    insert_site_query_prefixes_[prefix_index] =
    fmt::format("INSERT INTO {} (monthly_data_id, {}, "
                "population, clinical_episodes, ", site_table_name, location_id_column)
    + age_class_columns + age_columns +
    "treatments, treatment_failures, eir, pfpr_under5, pfpr_2to10, pfpr_all, "
    "infected_individuals, non_treatment, under5_treatment, over5_treatment, "
    "progress_to_clinical_in_7d_total, "
    "progress_to_clinical_in_7d_recrudescence, "
    "progress_to_clinical_in_7d_new_infection, "
    "recrudescence_treatment, "
    "total_number_of_bites_by_location, "
    "total_number_of_bites_by_location_year, "
    "person_days_by_location_year, "
    "current_foi_by_location) VALUES";


    insert_genome_query_prefixes_[prefix_index] =
        fmt::format(R"""(
        INSERT INTO {}
        (monthly_data_id, {}, genome_id, occurrences,
        clinical_occurrences, occurrences_0to5, occurrences_2to10,
        weighted_occurrences)
        VALUES
      )""",
                    genome_table_name, location_id_column);
  } catch (const std::exception &ex) {
    spdlog::error("Error creating tables for level {}:\n{}", level_id, ex.what());
  }
}

void SQLiteValidationReporter::count_infections_for_location(int level_id, int location_id) {
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);
  std::vector<int> ageClasses = Model::get_config()->age_structure();
  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    for (unsigned int ac = 0; ac < ageClasses.size(); ac++) {
      for (auto &person : index->vPerson()[location_id][hs][ac]) {
        // Is the individual infected by at least one parasite?
        if (person->get_all_clonal_parasite_populations()->empty()) { continue; }

        monthly_site_data_by_level[level_id].infections_by_unit[unit_id]++;
      }
    }
  }
}

void SQLiteValidationReporter::calculate_and_build_up_site_data_insert_values(int monthId,
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

    double calculatedEir = (monthly_site_data_by_level[level_id].eir[unit_id] != 0)
                               ? (monthly_site_data_by_level[level_id].eir[unit_id]
                                  / monthly_site_data_by_level[level_id].population[unit_id])
                               : 0;
    double calculatedPfprUnder5 = (monthly_site_data_by_level[level_id].pfpr_under5[unit_id] != 0)
                                      ? (monthly_site_data_by_level[level_id].pfpr_under5[unit_id]
                                         / monthly_site_data_by_level[level_id].population[unit_id])
                                            * 100.0
                                      : 0;
    double calculatedPfpr2to10 = (monthly_site_data_by_level[level_id].pfpr2to10[unit_id] != 0)
                                     ? (monthly_site_data_by_level[level_id].pfpr2to10[unit_id]
                                        / monthly_site_data_by_level[level_id].population[unit_id])
                                           * 100.0
                                     : 0;
    double calculatedPfprAll = (monthly_site_data_by_level[level_id].pfpr_all[unit_id] != 0)
                                   ? (monthly_site_data_by_level[level_id].pfpr_all[unit_id]
                                      / monthly_site_data_by_level[level_id].population[unit_id])
                                         * 100.0
                                   : 0;

    std::string singleRow =
        fmt::format("({}, {}, {}, {}", monthId, unit_id,
                    monthly_site_data_by_level[level_id].population[unit_id],
                    monthly_site_data_by_level[level_id].clinical_episodes[unit_id]);

    // Append clinical episodes by age class
    for (const auto &episodes :
         monthly_site_data_by_level[level_id].clinical_episodes_by_age_class[unit_id]) {
      singleRow += fmt::format(", {}", episodes);
    }

    for (const auto &treatment :
         monthly_site_data_by_level[level_id].recrudescence_treatment_by_age_class[unit_id]) {
      singleRow += fmt::format(", {}", treatment);
    }

    for (const auto &episodes :
         monthly_site_data_by_level[level_id].clinical_episodes_by_age[unit_id]) {
      singleRow += fmt::format(", {}", episodes);
    }

    for (const auto &population :
         monthly_site_data_by_level[level_id].population_by_age[unit_id]) {
      singleRow += fmt::format(", {}", population);
    }

    for (const auto &immune :
         monthly_site_data_by_level[level_id].total_immune_by_age[unit_id]) {
      singleRow += fmt::format(", {}", immune);
    }

    for (const auto &treatment :
         monthly_site_data_by_level[level_id].recrudescence_treatment_by_age[unit_id]) {
      singleRow += fmt::format(", {}", treatment);
    }

    for (const auto &moi :
         monthly_site_data_by_level[level_id].multiple_of_infection[unit_id]) {
      singleRow += fmt::format(", {}", moi);
    }

    singleRow +=
        fmt::format(", {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
                    monthly_site_data_by_level[level_id].treatments[unit_id],
                    monthly_site_data_by_level[level_id].treatment_failures[unit_id],
                    calculatedEir,
                    calculatedPfprUnder5,
                    calculatedPfpr2to10,
                    calculatedPfprAll,
                    monthly_site_data_by_level[level_id].infections_by_unit[unit_id],
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

    insert_values.push_back(singleRow);
  }
}

// Collect and store monthly site data
// Aggregates data related to various site metrics and stores them in the
// database
void SQLiteValidationReporter::monthly_report_site_data(int monthId) {
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

    std::vector<int> ageClasses = Model::get_config()->age_structure();
    // Reset data structures for this admin level
    reset_site_data_structures(level_id, vector_size, ageClasses.size());

    // Collect data for this admin level
    for (auto location = 0; location < Model::get_config()->number_of_locations(); location++) {
      auto locationPopulation = static_cast<int>(Model::get_population()->size(location));
      if (locationPopulation == 0) continue;

      collect_site_data_for_location(location, level_id);
    }

    // Calculate and insert data for this admin level
    calculate_and_build_up_site_data_insert_values(monthId, level_id);
    insert_monthly_site_data(level_id, insert_values);
  }
}

void SQLiteValidationReporter::collect_site_data_for_location(int location_id, int level_id) {
  // Get admin unit for this location at this level
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);

  std::vector<int> ageClasses = Model::get_config()->age_structure();

  count_infections_for_location(level_id, location_id);

  auto locationPopulation = static_cast<int>(Model::get_population()->size(location_id));
  // Collect the simple data
  monthly_site_data_by_level[level_id].population[unit_id] += static_cast<int>(locationPopulation);

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

  for (auto ndx = 0; ndx < ageClasses.size(); ndx++) {
    // Collect the treatment by age class, following the 0-59 month convention
    // for under-5
    if (ageClasses[ndx] < 5) {
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

  for (auto age = 0;  age < 80; age++) {
    monthly_site_data_by_level[level_id].clinical_episodes_by_age[unit_id][age] +=
        Model::get_mdc()
            ->monthly_number_of_clinical_episode_by_location_age()[location_id][age];
  }

  for (auto age = 0;  age < 80; age++) {
    monthly_site_data_by_level[level_id].population_by_age[unit_id][age] +=
        Model::get_mdc()
            ->popsize_by_location_age()[location_id][age];
  }

  for (auto age = 0;  age < 80; age++) {
    monthly_site_data_by_level[level_id].total_immune_by_age[unit_id][age] +=
        Model::get_mdc()
            ->total_immune_by_location_age()[location_id][age];
  }

  for (auto ndx = 0; ndx < ageClasses.size(); ndx++) {
    monthly_site_data_by_level[level_id].recrudescence_treatment_by_age_class[unit_id][ndx] +=
        Model::get_mdc()
            ->monthly_number_of_recrudescence_treatment_by_location_age_class()[location_id][ndx];
  }

  for (auto age = 0;  age < 80; age++) {
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

    auto eirLocation = Model::get_mdc()->eir_by_location_year()[location_id].empty()
                           ? 0
                           : Model::get_mdc()->eir_by_location_year()[location_id].back();
    monthly_site_data_by_level[level_id].eir[unit_id] += (eirLocation * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr_under5[unit_id] +=
        (Model::get_mdc()->get_blood_slide_prevalence(location_id, 0, 5) * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr2to10[unit_id] +=
        (Model::get_mdc()->get_blood_slide_prevalence(location_id, 2, 10) * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr_all[unit_id] +=
      (Model::get_mdc()->blood_slide_prevalence_by_location()[location_id] * locationPopulation);
  }
}

void SQLiteValidationReporter::collect_genome_data_for_location(size_t location_id, int level_id) {
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);
  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto ageClasses = index->vPerson()[0][0].size();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    // Iterate over all the age classes
    for (unsigned int ac = 0; ac < ageClasses; ac++) {
      // Iterate over all the genotypes
      auto peopleInAgeClass = index->vPerson()[location_id][hs][ac];
      for (auto &person : peopleInAgeClass) {
        collect_genome_data_for_a_person(person, unit_id, level_id);
      }
    }
  }
}

void SQLiteValidationReporter::reset_site_data_structures(int level_id, int vector_size,
                                                       size_t numAgeClasses) {
  // reset the data structures
  monthly_site_data_by_level[level_id].eir.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr_under5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr2to10.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr_all.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].population.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].clinical_episodes.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].clinical_episodes_by_age_class.assign(
  vector_size, std::vector<int>(numAgeClasses, 0));
  monthly_site_data_by_level[level_id].clinical_episodes_by_age.assign(
  vector_size, std::vector<int>(80, 0));
  monthly_site_data_by_level[level_id].population_by_age.assign(
  vector_size, std::vector<int>(80, 0));
  monthly_site_data_by_level[level_id].total_immune_by_age.assign(
  vector_size, std::vector<double>(80, 0));
  monthly_site_data_by_level[level_id].recrudescence_treatment_by_age_class.assign(
  vector_size, std::vector<ul>(numAgeClasses, 0));
  monthly_site_data_by_level[level_id].recrudescence_treatment_by_age.assign(
  vector_size, std::vector<ul>(80, 0));
  monthly_site_data_by_level[level_id].multiple_of_infection.assign(
  vector_size, std::vector<int>(ModelDataCollector::NUMBER_OF_REPORTED_MOI, 0));
  monthly_site_data_by_level[level_id].treatments.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatment_failures.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].nontreatment.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatments_under5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatments_over5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_total.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_recrudescence.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].progress_to_clinical_in_7d_new_infection.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].recrudescence_treatment.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].total_number_of_bites_by_location.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].total_number_of_bites_by_location_year.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].person_days_by_location_year.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].current_foi_by_location.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].infections_by_unit.assign(vector_size, 0);
}

void SQLiteValidationReporter::reset_genome_data_structures(int level_id, int vector_size,
                                                         size_t numGenotypes) {
  // reset the data structures
  monthly_genome_data_by_level[level_id].occurrences.assign(vector_size,
                                                            std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].clinical_occurrences.assign(
      vector_size, std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_0_5.assign(vector_size,
                                                                std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_2_10.assign(vector_size,
                                                                 std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].weighted_occurrences.assign(
      vector_size, std::vector<double>(numGenotypes, 0));
}

void SQLiteValidationReporter::collect_genome_data_for_a_person(Person* person, int unit_id,
                                                             int level_id) {
  const auto numGenotypes = Model::get_config()->number_of_parasite_types();
  auto individual = std::vector<int>(numGenotypes, 0);
  // Get the person, press on if they are not infected
  auto &parasites = *person->get_all_clonal_parasite_populations();
  auto numClones = parasites.size();
  if (numClones == 0) { return; }

  // Note the age and clinical status of the person
  auto age = person->get_age();
  auto clinical = static_cast<int>(person->get_host_state() == Person::HostStates::CLINICAL);

  // Count the genotypes present in the individual
  for (unsigned int ndx = 0; ndx < numClones; ndx++) {
    auto* parasitePopulation = parasites[ndx];
    auto genotypeId = parasitePopulation->genotype()->genotype_id();
    monthly_genome_data_by_level[level_id].occurrences[unit_id][genotypeId]++;
    monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotypeId] +=
        (age <= 5) ? 1 : 0;
    monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotypeId] +=
        (age >= 2 && age <= 10) ? 1 : 0;
    individual[genotypeId]++;

    // Count a clinical occurrence if the individual has clinical
    // symptoms
    monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotypeId] += clinical;
  }

  // Update the weighted occurrences and reset the individual count
  for (unsigned int ndx = 0; ndx < numGenotypes; ndx++) {
    if (individual[ndx] == 0) { continue; }
    monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][ndx] +=
        (individual[ndx] / static_cast<double>(numClones));
  }
}

void SQLiteValidationReporter::build_up_genome_data_insert_values(int monthId, int level_id) {
  auto numGenotypes = Model::get_config()->number_of_parasite_types();

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

    for (auto genotype = 0; genotype < numGenotypes; genotype++) {
      if (monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype] == 0) {
        continue;
      }
      std::string singleRow = fmt::format(
          "({}, {}, {}, {}, {}, {}, {}, {})", monthId, unit_id, genotype,
          monthly_genome_data_by_level[level_id].occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotype],
          monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype]);

      insert_values.push_back(singleRow);
    }
  }
}

void SQLiteValidationReporter::monthly_report_genome_data(int monthId) {
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

    auto numGenotypes = Model::get_config()->number_of_parasite_types();

    reset_genome_data_structures(level_id, vector_size, numGenotypes);

    auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

    // Iterate over all locations
    for (auto location = 0; location < index->vPerson().size(); location++) {
      collect_genome_data_for_location(location, level_id);
    }

    build_up_genome_data_insert_values(monthId, level_id);

    if (insert_values.empty()) {
      spdlog::info(
          "No genotypes recorded in the simulation at timestep, "
          "{}",
          Model::get_scheduler()->current_time());
      continue;
    }

    insert_monthly_genome_data(level_id, insert_values);
  }
}

void SQLiteValidationReporter::after_run() {
  SQLiteDbReporter::after_run();
}
