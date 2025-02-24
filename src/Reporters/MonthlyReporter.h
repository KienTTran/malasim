#ifndef POMS_BFREPORTER_H
#define POMS_BFREPORTER_H

#include <fstream>
#include <sstream>

#include "Reporter.h"

class MonthlyReporter : public Reporter {
public:
  //disable copy and assignment and move
  MonthlyReporter(const MonthlyReporter &) = delete;
  void operator=(const MonthlyReporter &) = delete;
  MonthlyReporter(MonthlyReporter &&) = delete;
  void operator=(MonthlyReporter &&) = delete;

public:
  const std::string group_sep = "-1111\t";
  const std::string sep = "\t";
  std::ofstream gene_freq_file;
  std::ofstream monthly_data_file;
  std::ofstream summary_data_file;
  std::ofstream gene_db_file;

public:
  MonthlyReporter();

  ~MonthlyReporter() override;

  void initialize(int job_number, const std::string &path) override;

  void before_run() override;

  void after_run() override;

  void begin_time_step() override;

  void monthly_report() override;

  void print_EIR_PfPR_by_location(std::stringstream& ss);

  //  void print_monthly_incidence_by_location();
};

#endif  // POMS_BFREPORTER_H
