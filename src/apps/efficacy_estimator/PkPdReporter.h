/*
 * File:   PkPdReporter.h
 * Author: Merlin
 *
 * Created on October 29, 2014, 12:56 PM
 */

#ifndef PKPDREPORTER_H
#define PKPDREPORTER_H

#include <fstream>

#include "apps/efficacy_estimator/DxGAppInput.h"
#include "Reporters/Reporter.h"

class PkPdReporter : public Reporter {
public:
  explicit PkPdReporter(utils::DxGAppInput* app_input = nullptr);

  PkPdReporter(const PkPdReporter &) = delete;
  PkPdReporter(PkPdReporter &&) = delete;
  PkPdReporter &operator=(const PkPdReporter &) = delete;
  PkPdReporter &operator=(PkPdReporter &&) = delete;
  PkPdReporter(std::string prefix, std::ofstream parasitaemia_file)
      : prefix_(std::move(prefix)), parasitaemia_file_(std::move(parasitaemia_file)) {}
  ~PkPdReporter() override;

  void initialize(int job_number, const std::string &path) override;

  void before_run() override;

  void after_run() override;

  void begin_time_step() override;

  void after_time_step() override;

  void monthly_report() override;

private:
  utils::DxGAppInput* app_input_{nullptr};
  std::string prefix_;
  std::ofstream parasitaemia_file_;
};

#endif /* PKPDREPORTER_H */
