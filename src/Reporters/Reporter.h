#ifndef REPORTER_H
#define REPORTER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

class Model;
class Genotype;

// Wrapper for TSV file constants
namespace tsv {
const std::string SEP = "\t";
const std::string GROUP_SEP = "-1111\t";
const std::string END_LINE = "\n";
const std::string EXTENSION = "tsv";
}  // namespace tsv

// Wrapper for CSV file constants
namespace csv {
const std::string SEP = ",";
const std::string END_LINE = "\n";
const std::string EXTENSION = "csv";
}  // namespace csv

class Reporter {
public:
  // Disallow copy
  Reporter(const Reporter &) = delete;
  Reporter &operator=(const Reporter &) = delete;

  // Disallow move
  Reporter(Reporter &&) = delete;
  Reporter &operator=(Reporter &&) = delete;

  void set_model(Model* value) { model_ = value; }

private:
  Model* model_;

public:
  enum class ReportType : std::uint8_t {
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
    SQLITE_VALIDATION_REPORTER,

#ifdef ENABLE_TRAVEL_TRACKING
    TRAVEL_TRACKING_REPORTER,
#endif
  };

  static std::map<std::string, ReportType> report_type_map;

  Reporter();

  //    Reporter(const Reporter& orig);
  virtual ~Reporter();

  virtual void initialize(int job_number, const std::string &path) = 0;

  virtual void before_run() = 0;

  virtual void after_run() = 0;

  virtual void begin_time_step() = 0;

  virtual void after_time_step() {}

  virtual void monthly_report() = 0;

  virtual void on_genotype_added(const Genotype & /*genotype*/) {}

  // Returns nullptr (and logs an error) if `report_type` has no implementation.
  // Callers must check the result before passing it to Model::add_reporter().
  static std::unique_ptr<Reporter> make_report(ReportType report_type);

  // Names accepted by the `-r/--reporter` command line option, in map order.
  static std::vector<std::string> available_reporters();

private:
};

#endif /* REPORTER_H */
