/*
 * SQLiteDbReporter.h
 *
 * Define the SQLiteDbReporter class which is used to insert relevant
 * information from the model into the database during model execution.
 */
#ifndef SQLITEPIXELREPORTER_H
#define SQLITEPIXELREPORTER_H

#include "Reporters/SQLiteDbReporter.h"

class SQLitePixelReporter : public SQLiteDbReporter {
public:
  //disallow copy and move
  SQLitePixelReporter(const SQLitePixelReporter &) = delete;
  SQLitePixelReporter(SQLitePixelReporter &&) = delete;
  SQLitePixelReporter &operator=(SQLitePixelReporter &&) = delete;

protected:
  // Return the character code that indicates the level of genotype records (c:
  // cell, d: district)
  char get_genotype_level() override { return 'C'; }
  void monthly_report_genome_data(int monthId) override;
  void monthly_report_site_data(int monthId) override;

public:
  SQLitePixelReporter() = default;
  ~SQLitePixelReporter() override = default;

  // Overrides
  void initialize(int job_number, const std::string &path) override;
};

#endif
