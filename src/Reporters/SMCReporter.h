#ifndef REPORTERS_SMCREPORTER_H
#define REPORTERS_SMCREPORTER_H

//#ifdef ENABLE_SMC

#include <fstream>

#include "Reporters/Reporter.h"

class SMCReporter : public Reporter {
  
private:
  std::ofstream output_file;

public:

  // Disallow copy
  SMCReporter(const SMCReporter&) = delete;
  SMCReporter& operator=(const SMCReporter&) = delete;

  // Disallow move
  SMCReporter(SMCReporter&&) = delete;
  SMCReporter& operator=(SMCReporter&&) = delete;

  
  SMCReporter();
  ~SMCReporter() override;

  void initialize(int job_number, const std::string &path) override;
  void before_run() override;
  void after_run() override;
  void begin_time_step() override;
  void monthly_report() override;

  int get_first_smc_month() const;

  void custom_report();
};

//#endif  // ENABLE_SMC
#endif  // REPORTERS_SMCREPORTER_H
