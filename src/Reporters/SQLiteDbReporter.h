/*
 * SQLiteDbReporter.h
 *
 * SQLite implementation of the reporter that logs simulation data including
 * genotype information at different administrative levels.
 */
#ifndef SQLITEDBREPORTER_H
#define SQLITEDBREPORTER_H

#include "Utils/Helpers/SQLiteDatabase.h"
#include "Reporter.h"
#include <memory>
#include <string>
#include <vector>

class SQLiteDbReporter : public Reporter {
private:
  // SQL query templates
  const std::string insert_genotype_query_ =
      "INSERT INTO genotype (id, name) VALUES (?, ?);";

  const std::string insert_admin_level_query_ =
      "INSERT INTO admin_level (id, name) VALUES (?, ?);";

  const std::string insert_location_admin_map_query_ =
      "INSERT INTO location_admin_map (location_id, admin_level_id, admin_unit_id) VALUES (?, ?, ?);";

  const std::string insert_common_query_ = R""""(
  INSERT INTO monthly_data (days_elapsed, model_time, seasonal_factor)
  VALUES (?, ?, ?)
  RETURNING id;
  )"""";

  // Dynamically generated query prefixes for each admin level
  std::vector<std::string> insert_site_query_prefixes_;
  std::vector<std::string> insert_genome_query_prefixes_;

  // Database schema management
  void create_all_reporting_tables();
  void create_reporting_tables_for_level(int level_id, const std::string& ageClassColumnDefinitions, const std::string& ageClassColumns);
  void populate_db_schema();
  void populate_genotype_table();
  void populate_admin_level_table();
  void populate_location_admin_map_table();

  // Utility methods for table names
  std::string get_site_table_name(int level_id) const;
  std::string get_genome_table_name(int level_id) const;


protected:
  // Special level_id for cell-level data
  int CELL_LEVEL_ID = -1;
  // Database connection
  std::unique_ptr<SQLiteDatabase> db;

  // Constants for batch size
  static constexpr int DEFAULT_BATCH_SIZE = 1000;
  int batch_size = DEFAULT_BATCH_SIZE;

  // Virtual methods to be implemented by derived classes
  virtual void monthly_report_genome_data(int month_id) = 0;
  virtual void monthly_report_site_data(int month_id) = 0;

  // Data insertion helpers - using level_id = CELL_LEVEL_ID for cell data
  void insert_monthly_site_data(int level_id, const std::vector<std::string> &site_data);
  void insert_monthly_genome_data(int level_id, const std::vector<std::string> &genome_data);

  // Batch insertion helpers
  void batch_insert_query(const std::string &query_prefix, const std::vector<std::string> &values);

  // Utility method
  int get_admin_level_count() const;

public:
  // Constructor and destructor
  SQLiteDbReporter() = default;
  ~SQLiteDbReporter() override = default;

  // Delete copy and move operations
  SQLiteDbReporter(const SQLiteDbReporter &) = delete;
  SQLiteDbReporter(SQLiteDbReporter &&) = delete;
  SQLiteDbReporter &operator=(const SQLiteDbReporter &) = delete;
  SQLiteDbReporter &operator=(SQLiteDbReporter &&) = delete;

  // Reporter interface implementation
  void initialize(int job_number, const std::string &path) override;
  void before_run() override {}
  void begin_time_step() override {}
  void monthly_report() override;
  void after_run() override {
    populate_genotype_table();
  }

  // Set batch size for database operations
  void set_batch_size(int size) { batch_size = size > 0 ? size : DEFAULT_BATCH_SIZE; }
  int get_batch_size() const { return batch_size; }
};

#endif // SQLITEDBREPORTER_H
