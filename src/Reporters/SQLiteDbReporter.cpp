#include "SQLiteDbReporter.h"

#include <filesystem>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Spatial/GIS/SpatialData.h"
#include "Utils/Helpers/StringHelpers.h"

// Function to populate the 'genotype' table in the database
void SQLiteDbReporter::populate_genotype_table() {
  try {
    // Use the Database class to execute SQL statements
    db->execute("DELETE FROM genotype;");  // Clear the genotype table

    auto* config = Model::get_config();
    std::vector<std::string> batch_values;
    batch_values.reserve(config->number_of_parasite_types());

    for (auto id = 0; id < config->number_of_parasite_types(); id++) {
      auto* genotype = Model::get_genotype_db()->at(id);
      // Escape single quotes in genotype string if needed
      std::string genotype_str = genotype->get_aa_sequence();
      StringHelpers::replace_all(genotype_str, "'", "''");

      batch_values.push_back(fmt::format("({}, '{}')", id, genotype_str));
    }

    // Insert in batches
    std::string query_prefix = "INSERT INTO genotype (id, name) VALUES ";
    batch_insert_query(query_prefix, batch_values);

  } catch (const std::exception &ex) { spdlog::error("{}:\n{}", __FUNCTION__, ex.what()); }
}

// Function to populate the 'admin_level' table in the database
void SQLiteDbReporter::populate_admin_level_table() {
  try {
    // Clear the admin_level table
    db->execute("DELETE FROM admin_level;");

    // Get admin level names from SpatialData
    auto admin_levels = Model::get_spatial_data()->get_admin_level_manager()->get_level_names();

    if (admin_levels.empty()) {
      spdlog::warn("No admin levels found. Skipping admin_level table population.");
      return;
    }

    std::vector<std::string> batch_values;
    batch_values.reserve(admin_levels.size());

    for (size_t id = 0; id < admin_levels.size(); id++) {
      // Escape single quotes if needed
      std::string level_name = admin_levels[id];
      StringHelpers::replace_all(level_name, "'", "''");

      batch_values.push_back(fmt::format("({}, '{}')", id, level_name));
    }

    // Insert in batches
    std::string query_prefix = "INSERT INTO admin_level (id, name) VALUES ";
    batch_insert_query(query_prefix, batch_values);

  } catch (const std::exception &ex) { spdlog::error("{}:\n{}", __FUNCTION__, ex.what()); }
}

// Function to create tables for each admin level
void SQLiteDbReporter::create_all_reporting_tables() {
  spdlog::info("SQLiteDbReporter Creating all reporting tables");
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

  for (size_t i = 0; i < Model::get_config()->get_epidemiological_parameters().get_not_progress_to_clinical().size(); ++i) {
    age_column_definitions += fmt::format("number_of_not_progress_to_clinical_threshold_{} INTEGER, ", i);
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
  for (size_t i = 0; i < Model::get_config()->get_epidemiological_parameters().get_not_progress_to_clinical().size(); ++i) {
    age_columns += fmt::format("number_of_not_progress_to_clinical_threshold_{}, ", i);
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

void SQLiteDbReporter::create_reporting_tables_for_level(
    int level_id, const std::string &age_class_column_definitions,
    const std::string &age_class_columns,
    const std::string &age_column_definitions,
    const std::string &age_columns) {
  spdlog::info("SQLiteDbReporter Creating reporting tables for level");
  // Generate table names for this level
  std::string site_table_name = get_site_table_name(level_id);
  std::string genome_table_name = get_genome_table_name(level_id);

  // location_id for cell level or unit_id for admin level
  std::string location_id_column = (level_id == CELL_LEVEL_ID) ? "location_id" : "unit_id";

  // Create site data table for this level
  std::string drop_site_data_table = fmt::format("DROP TABLE IF EXISTS {}", site_table_name);
  db->execute(drop_site_data_table);

  std::string create_site_data_table = fmt::format(R""""(
      CREATE TABLE {} (
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
  std::string drop_genome_data_table = fmt::format("DROP TABLE IF EXISTS {}", genome_table_name);
  db->execute(drop_genome_data_table);

  std::string create_genome_data_table =
      fmt::format(R""""(
      CREATE TABLE {} (
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
      "treatments, eir, pfpr_under5, pfpr_2to10, pfpr_all, "
      "infected_individuals, treatment_failures, "
      "non_treatment, under5_treatment, over5_treatment, "
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

// Function to populate the 'location_admin_map' table in the database
void SQLiteDbReporter::populate_location_admin_map_table() {
  try {
    // Clear the table
    db->execute("DELETE FROM location_admin_map;");

    auto* spatial_data = Model::get_spatial_data();
    auto &location_db = Model::get_config()->location_db();
    auto* admin_level_manager = spatial_data->get_admin_level_manager();

    // Prepare for batch insertion
    std::vector<std::string> batch_values;
    batch_values.reserve(location_db.size() * admin_level_manager->get_level_count());

    // For each location in the location database
    for (int location_id = 0; location_id < location_db.size(); location_id++) {
      // For each admin level
      for (int admin_level_id = 0; admin_level_id < admin_level_manager->get_level_count();
           admin_level_id++) {
        // Get admin unit ID for this location at this admin level
        auto admin_unit_id = spatial_data->get_admin_unit(admin_level_id, location_id);

        // Add to batch values
        batch_values.push_back(
            fmt::format("({}, {}, {})", location_id, admin_level_id, admin_unit_id));
      }
    }

    // Insert in batches
    std::string query_prefix =
        "INSERT INTO location_admin_map (location_id, admin_level_id, admin_unit_id) VALUES ";
    batch_insert_query(query_prefix, batch_values);

  } catch (const std::exception &ex) { spdlog::error("{}:\n{}", __FUNCTION__, ex.what()); }
}

// Function to create the database schema
// This sets up the necessary tables in the database
void SQLiteDbReporter::populate_db_schema() {
  // Create the table schema
  const std::string createMonthlyData = R""""(
    CREATE TABLE IF NOT EXISTS monthly_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        days_elapsed INTEGER NOT NULL,
        model_time INTEGER NOT NULL,
        seasonal_factor INTEGER NOT NULL
    );
  )"""";

  const std::string createGenotype = R""""(
    CREATE TABLE IF NOT EXISTS genotype (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
  )"""";

  const std::string createAdminLevel = R""""(
    CREATE TABLE IF NOT EXISTS admin_level (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
  )"""";

  const std::string createLocationAdminMap = R""""(
    CREATE TABLE IF NOT EXISTS location_admin_map (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        location_id INTEGER NOT NULL,
        admin_level_id INTEGER NOT NULL,
        admin_unit_id INTEGER NOT NULL,
        FOREIGN KEY (admin_level_id) REFERENCES admin_level(id)
    );
    CREATE INDEX IF NOT EXISTS idx_admin_unit ON location_admin_map (admin_level_id, admin_unit_id);
    CREATE UNIQUE INDEX IF NOT EXISTS idx_location_admin ON location_admin_map (location_id, admin_level_id);
  )"""";

  try {
    TransactionGuard tx(db.get());
    // Use the Database class to execute SQL statements
    db->execute(createMonthlyData);
    db->execute(createGenotype);
    db->execute(createAdminLevel);
    db->execute(createLocationAdminMap);

    // Create tables for all reporting levels (cell and admin)
    create_all_reporting_tables();

  } catch (const std::exception &ex) {
    spdlog::error("Error in populate_db_schema:\n{}", ex.what());
    // Consider more robust error handling rather than simply logging
  }
}

// Initialize the reporter
// Sets up the database and prepares it for data entry
void SQLiteDbReporter::initialize(int jobNumber, const std::string &path) {
  spdlog::info("Base SQLiteDbReporter initialized.");

  // Define the database file path
  auto dbPath = fmt::format("{}monthly_data_{}.db", path, jobNumber);

  // Check if the file exists
  if (std::filesystem::exists(dbPath)) {
    // Delete the old database file if it exists
    if (std::remove(dbPath.c_str()) != 0) {
      // Handle the error, if any, when deleting the old file
      spdlog::error("Error deleting old database file.");
    }
  } else {
    // The file doesn't exist, so no need to delete it
    spdlog::info("Database file does not exist. No deletion needed.\n");
  }

  // Open or create the SQLite database file
  db = std::make_unique<SQLiteDatabase>(dbPath);

  // Get number of admin levels to initialize vectors
  int admin_level_count =
      Model::get_spatial_data()->get_admin_level_manager()->get_level_names().size();

  // Include cell level in the number of levels
  insert_site_query_prefixes_.resize(admin_level_count + 1);
  insert_genome_query_prefixes_.resize(admin_level_count + 1);

  // Update cell level id
  CELL_LEVEL_ID = admin_level_count;

  populate_db_schema();
  // populate the genotype table data
  populate_genotype_table();
  // populate the admin level table data
  populate_admin_level_table();
  // populate the location admin map table data
  populate_location_admin_map_table();

  std::string ageClassColumns;
  for (auto ndx = 0; ndx < Model::get_config()->age_structure().size(); ndx++) {
    auto agFrom = ndx == 0 ? 0 : Model::get_config()->age_structure()[ndx - 1];
    auto agTo = Model::get_config()->age_structure()[ndx];
    ageClassColumns += fmt::format("clinical_episodes_by_age_class_{}_{}, ", agFrom, agTo);
  }
}

void SQLiteDbReporter::monthly_report() {
  // Get the relevant data
  auto daysElapsed = Model::get_scheduler()->current_time();
  auto modelTime = Model::get_scheduler()->get_unix_time();
  auto seasonalFactor = Model::get_config()->get_seasonality_settings().get_seasonal_factor(
      Model::get_scheduler()->get_calendar_date(), 0);

  auto monthId = db->insert_data(insert_common_query_, daysElapsed, modelTime, seasonalFactor);

  monthly_report_site_data(monthId);
  if (Model::get_config()->get_model_settings().get_record_genome_db()
      && Model::get_mdc()->recording_data()) {
    // Add the genome information, this will also update infected individuals
    monthly_report_genome_data(monthId);
  }
}

void SQLiteDbReporter::batch_insert_query(const std::string &query_prefix,
                                          const std::vector<std::string> &values) {
  if (values.empty()) return;

  // Process in batches
  for (size_t i = 0; i < values.size(); i += batch_size) {
    size_t end = std::min(i + batch_size, values.size());
    std::vector<std::string> batch_values(values.begin() + i, values.begin() + end);

    // Create batch query
    std::string query = query_prefix + StringHelpers::join(batch_values, ",") + ";";
    db->execute(query);
  }
}

void SQLiteDbReporter::insert_monthly_site_data(int level_id,
                                                const std::vector<std::string> &siteData) {
  // Skip if empty
  if (siteData.empty()) return;

  // For cell level, use the last index in the query prefix vector
  int query_index = (level_id == CELL_LEVEL_ID) ? insert_site_query_prefixes_.size() - 1 : level_id;

  // Insert the site data into the database using batch insertion
  batch_insert_query(insert_site_query_prefixes_[query_index], siteData);
}

void SQLiteDbReporter::insert_monthly_genome_data(int level_id,
                                                  const std::vector<std::string> &genomeData) {
  // Skip if empty
  if (genomeData.empty()) return;

  // For cell level, use the last index in the query prefix vector
  int query_index =
      (level_id == CELL_LEVEL_ID) ? insert_genome_query_prefixes_.size() - 1 : level_id;

  // Insert the genome data into the database using batch insertion
  batch_insert_query(insert_genome_query_prefixes_[query_index], genomeData);
}

std::string SQLiteDbReporter::get_site_table_name(int level_id) const {
  if (level_id == CELL_LEVEL_ID) { return "monthly_site_data_cell"; }
  return "monthly_site_data_" + Model::get_spatial_data()->get_admin_level_name(level_id);
}

std::string SQLiteDbReporter::get_genome_table_name(int level_id) const {
  if (level_id == CELL_LEVEL_ID) { return "monthly_genome_data_cell"; }
  return "monthly_genome_data_" + Model::get_spatial_data()->get_admin_level_name(level_id);
}

const std::string insert_location_admin_map_query_ =
    "INSERT INTO location_admin_map (location_id, admin_level_id, admin_unit_id) VALUES (?, ?, ?);";

