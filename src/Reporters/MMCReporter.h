
#ifndef MMCREPORTER_H
#define MMCREPORTER_H

#include "Reporter.h"
#include <sstream>

class PersonIndexByLocationStateAgeClass;

class MMCReporter : public Reporter {
public:
  //disallow copy, assign and move
  MMCReporter(const MMCReporter &) = delete;
  void operator=(const MMCReporter &) = delete;
  MMCReporter(MMCReporter &&) = delete;
  MMCReporter operator=(MMCReporter &&) = delete;

public:
  std::stringstream ss;
  const std::string group_sep = "-1111\t";
  const std::string sep = "\t";

  MMCReporter();

  ~MMCReporter() override = default;

  void initialize(int job_number, const std::string &path) override;

  void before_run() override;

  void after_run() override;

  void begin_time_step() override;

  void print_treatment_failure_rate_by_therapy();

  void print_ntf_by_location();

  void print_genotype_frequency();

  void monthly_report() override;

  void print_EIR_PfPR_by_location();

};

#endif // MMCREPORTER_H
