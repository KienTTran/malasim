/*
 * File:   IndividualsFileReporter.h
 * Author: Merlin
 *
 * Created on November 7, 2014, 11:06 AM
 */

#ifndef INDIVIDUALSFILEREPORTER_H
#define INDIVIDUALSFILEREPORTER_H

#include <fstream>
#include <string>

#include "Reporters/Reporter.h"

class IndividualsFileReporter : public Reporter {
  std::fstream fs_;
  std::string file_name_;

public:
  IndividualsFileReporter(const IndividualsFileReporter &) = delete;
  IndividualsFileReporter(IndividualsFileReporter &&) = delete;
  IndividualsFileReporter &operator=(const IndividualsFileReporter &) = delete;
  IndividualsFileReporter &operator=(IndividualsFileReporter &&) = delete;
  explicit IndividualsFileReporter(std::string file_name);

  //    IndividualsFileReporter(const IndividualsFileReporter& orig);
  ~IndividualsFileReporter() override;

private:
  void initialize(int job_number, const std::string &path) override {}

  void before_run() override;

  void after_run() override;

  void begin_time_step() override;

  void after_time_step() override;

public:
  void monthly_report() override;
};

#endif /* INDIVIDUALSFILEREPORTER_H */
