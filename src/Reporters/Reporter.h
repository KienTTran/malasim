#ifndef REPORTER_H
#define REPORTER_H

#include <map>
#include <string>

class Model;

// Wrapper for TSV file constants
namespace Tsv {
const std::string sep = "\t";
const std::string end_line = "\n";
const std::string extension = "tsv";
}  // namespace Tsv

// Wrapper for CSV file constants
namespace Csv {
const std::string sep = ",";
const std::string end_line = "\n";
const std::string extension = "csv";
}  // namespace Csv

class Reporter {
public:
  //disallow copy and assign and move
  Reporter(const Reporter &orig) = delete;
  void operator=(const Reporter &orig) = delete;
  Reporter(Reporter &&orig) = delete;
  Reporter operator=(Reporter &&orig) = delete;

public:
  Model *model{};
  void set_model(Model *value) { model = value; }

 private:
 Model *model_;

public:
  enum ReportType {
    CONSOLE,
    MONTHLY_REPORTER,
    MMC_REPORTER,
    TACT_REPORTER,
    NOVEL_DRUG_REPOTER,
    VALIDATION_REPORTER,

    // Specialist reporters for specific experiments
    MOVEMENT_REPORTER,
    POPULATION_REPORTER,
    CELLULAR_REPORTER,
    GENOTYPE_CARRIERS,
    SEASONAL_IMMUNITY,
    AGE_BAND_REPORTER,
    THERAPY_RECORD_REPORTER,

    // SQLite reporter
    // SQLITE_DISTRICT_REPORTER,
    // SQLITE_PIXEL_REPORTER,
    SQLITE_MONTHLY_REPORTER,

#ifdef ENABLE_TRAVEL_TRACKING
    TRAVEL_TRACKING_REPORTER,
#endif
  };



 public:
  static std::map<std::string, ReportType> ReportTypeMap;

 public:
  Reporter();

  //    Reporter(const Reporter& orig);
  virtual ~Reporter();

  virtual void initialize(int job_number, const std::string &path) = 0;

  virtual void before_run() = 0;

  virtual void after_run() = 0;

  virtual void begin_time_step() = 0;

  virtual void monthly_report() = 0;

  static Reporter *MakeReport(ReportType report_type);

 private:

};

#endif /* REPORTER_H */
