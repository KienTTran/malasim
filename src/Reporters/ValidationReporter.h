#ifndef VALIDATIONREPORTER_H
#define VALIDATIONREPORTER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include "Reporter.h"

class ValidationReporter : public Reporter {
public:
  // Disable copy, assignment, and move
  ValidationReporter(const ValidationReporter &) = delete;
  void operator=(const ValidationReporter &) = delete;
  ValidationReporter(ValidationReporter &&) = delete;
  ValidationReporter operator=(ValidationReporter &&) = delete;

public:
  const std::string group_sep = "-1111\t";
  const std::string sep = "\t";

  std::shared_ptr<spdlog::logger> monthly_data_logger;
  std::shared_ptr<spdlog::logger> summary_data_logger;
  std::shared_ptr<spdlog::logger> gene_db_logger;
  std::shared_ptr<spdlog::logger> gene_freq_logger;
  // std::shared_ptr<spdlog::logger> prmc_db_logger;
  // std::shared_ptr<spdlog::logger> prmc_freq_logger;
  std::shared_ptr<spdlog::logger> monthly_mutation_logger;
  std::shared_ptr<spdlog::logger> mosquito_res_count_logger;

public:
  ValidationReporter();
  ~ValidationReporter() override = default;

  void initialize(int job_number, const std::string &path) override;
  void before_run() override;
  void after_run() override;
  void begin_time_step() override;
  void monthly_report() override;
  void print_EIR_PfPR_by_location(std::stringstream& ss);
};

#endif  // VALIDATIONREPORTER_H
