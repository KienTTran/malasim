#include "SQLiteDbReporter.h"

#include <filesystem>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Utils/Helpers/StringHelpers.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Simulation/Model.h"
#include "Population/Population.h"
#include "Utils/Cli.hxx"

// Function to populate the 'genotype' table in the database
void SQLiteDbReporter::populate_genotype_table() {
  try {
    // Use the Database class to execute and prepare SQL statements
    db->execute("DELETE FROM genotype;");  // Clear the genotype table

    // Prepare the bulk query
    auto* stmt = db->prepare(insert_genotype_query_);

    auto* config = Model::get_config();

    for (auto id = 0; id < config->get_genotype_parameters().genotype_db->size(); id++) {
      Genotype* genotype = config->get_genotype_parameters().genotype_db->get_genotype_by_id(id);
      // Bind values to the prepared statement
      sqlite3_bind_int(stmt, 1, id);
      sqlite3_bind_text(stmt, 2, genotype->get_aa_sequence().c_str(), strlen(genotype->get_aa_sequence().c_str()), SQLITE_TRANSIENT);

      if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::runtime_error("Error executing INSERT statement");
      }

      sqlite3_reset(stmt);  // Reset the statement for the next iteration
    }

    sqlite3_finalize(stmt);  // Finalize the statement

  } catch (const std::exception &ex) {
    spdlog::error("{}-{}", __FUNCTION__, ex.what());
  }
}

// Function to create the database schema
// This sets up the necessary tables in the database
void SQLiteDbReporter::populate_db_schema() {
  // Create the table schema
  const std::string createMonthlyData = R""""(
    CREATE TABLE IF NOT EXISTS monthlydata (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        dayselapsed INTEGER NOT NULL,
        modeltime INTEGER NOT NULL,
        seasonalfactor INTEGER NOT NULL
    );
  )"""";

  std::string ageClassColumns;
  for (auto ndx = 0; ndx < Model::get_config()->get_population_demographic().get_age_structure().size(); ndx++) {
    auto agFrom = ndx == 0 ? 0 : Model::get_config()->get_population_demographic().get_age_structure()[ndx - 1];
    auto agTo = Model::get_config()->get_population_demographic().get_age_structure()[ndx];
    ageClassColumns +=
        fmt::format("clinicalepisodes_by_age_class_{}_{}, ", agFrom, agTo);
  }

  const std::string createMonthlySiteData =
      R""""(
    CREATE TABLE IF NOT EXISTS monthlysitedata (
        monthlydataid INTEGER NOT NULL,
        locationid INTEGER NOT NULL,
        population INTEGER NOT NULL,
        clinicalepisodes INTEGER NOT NULL, )""""
      + ageClassColumns +
      R""""(
        treatments INTEGER NOT NULL,
        treatmentfailures INTEGER NOT NULL,
        eir REAL NOT NULL,
        pfprunder5 REAL NOT NULL,
        pfpr2to10 REAL NOT NULL,
        pfprall REAL NOT NULL,
        infectedindividuals INTEGER,
        nontreatment INTEGER NOT NULL,
        under5treatment INTEGER NOT NULL,
        over5treatment INTEGER NOT NULL,
        PRIMARY KEY (monthlydataid, locationid),
        FOREIGN KEY (monthlydataid) REFERENCES monthlydata(id)
    );
  )"""";

  const std::string createGenotype = R""""(
    CREATE TABLE IF NOT EXISTS genotype (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
  )"""";

  const std::string createMonthlyGenomeData = R""""(
    CREATE TABLE IF NOT EXISTS monthlygenomedata (
        monthlydataid INTEGER NOT NULL,
        locationid INTEGER NOT NULL,
        genomeid INTEGER NOT NULL,
        occurrences INTEGER NOT NULL,
        clinicaloccurrences INTEGER NOT NULL,
        occurrences0to5 INTEGER NOT NULL,
        occurrences2to10 INTEGER NOT NULL,
        weightedoccurrences REAL NOT NULL,
        PRIMARY KEY (monthlydataid, genomeid, locationid),
        FOREIGN KEY (genomeid) REFERENCES genotype(id),
        FOREIGN KEY (monthlydataid) REFERENCES monthlydata(id)
    );
  )"""";

  try {
    TransactionGuard tx(db.get());
    // Use the Database class to execute SQL statements
    db->execute(createMonthlyData);
    db->execute(createMonthlySiteData);
    db->execute(createGenotype);
    db->execute(createMonthlyGenomeData);
  } catch (const std::exception &ex) {
    spdlog::error("Error in populate_db_schema: {}", ex.what());
    // Consider more robust error handling rather than simply logging
  }
}

// Initialize the reporter
// Sets up the database and prepares it for data entry
void SQLiteDbReporter::initialize(int job_number, const std::string &path) {
  spdlog::info("Base SQLiteDbReporter initialized.");

  // Define the database file path
  auto dbPath = fmt::format("{}monthly_data_{}.db",path,job_number);

  // Check if the file exists
  if (std::filesystem::exists(dbPath)) {
    // Delete the old database file if it exists
    if (std::remove(dbPath.c_str()) != 0) {
      // Handle the error, if any, when deleting the old file
      spdlog::error("Error deleting old database file.");
    }
  } else {
    // The file doesn't exist, so no need to delete it
    spdlog::info("Database file does not exist. No deletion needed.");
  }

  // Open or create the SQLite database file
  db = std::make_unique<SQLiteDatabase>(dbPath);

  populate_db_schema();
  // populate the genotype table
  populate_genotype_table();

  std::string ageClassColumns;
  for (auto ndx = 0; ndx < Model::get_config()->get_population_demographic().get_age_structure().size(); ndx++) {
    auto agFrom = ndx == 0 ? 0 : Model::get_config()->get_population_demographic().get_age_structure()[ndx - 1];
    auto agTo = Model::get_config()->get_population_demographic().get_age_structure()[ndx];
    ageClassColumns +=
        fmt::format("clinicalepisodes_by_age_class_{}_{}, ", agFrom, agTo);
  }

  // craete insert query based on the age class config
  insert_site_query_prefix_ = " INSERT INTO MonthlySiteData (MonthlyDataId, LocationId, "
    "Population, ClinicalEpisodes, " 
    + ageClassColumns + 
    " Treatments, EIR, PfPrUnder5, PfPr2to10, PfPrAll, infectedindividuals , TreatmentFailures,"
    " NonTreatment, Under5Treatment, Over5Treatment) VALUES";
}

void SQLiteDbReporter::monthly_report() {
  // Get the relevant data
  auto daysElapsed = Model::get_scheduler()->current_time();
  auto modelTime =
      std::chrono::system_clock::to_time_t(Model::get_scheduler()->calendar_date);
  auto seasonalFactor = Model::get_config()->get_seasonality_settings().get_seasonal_factor(
      Model::get_scheduler()->calendar_date, 0);

  auto monthId = db->insert_data(insert_common_query_, daysElapsed, modelTime,
                                 seasonalFactor);

  monthly_report_site_data(monthId);
  if (Model::get_config()->get_model_settings().get_record_genome_db()
      && Model::get_mdc()->recording_data()) {
    // Add the genome information, this will also update infected individuals
    monthly_report_genome_data(monthId);
  }
}

void SQLiteDbReporter::insert_monthly_site_data(
    const std::vector<std::string> &siteData) {
  // Insert the site data into the database
  std::string const query =
      insert_site_query_prefix_ + StringHelpers::join(siteData, ",") + ";";
  db->execute(query);
}

void SQLiteDbReporter::insert_monthly_genome_data(
    const std::vector<std::string> &genomeData) {
  // Insert the genome data into the database
  std::string const query = insert_genotype_query_prefix_
                            + StringHelpers::join(genomeData, ",") + ";";
  db->execute(query);
}

