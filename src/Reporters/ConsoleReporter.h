#ifndef CONSOLEREPORTER_H
#define    CONSOLEREPORTER_H

#include "Reporter.h"

class ConsoleReporter : public Reporter {
public:
  //disallow copy, assign and move
  ConsoleReporter(const ConsoleReporter &) = delete;
  void operator=(const ConsoleReporter &) = delete;
  ConsoleReporter(ConsoleReporter &&) = delete;
  ConsoleReporter operator=(ConsoleReporter &&) = delete;

 public:
  ConsoleReporter();

  //    ConsoleReporter(const ConsoleReporter& orig);
  virtual ~ConsoleReporter();

  virtual void initialize(int job_number, const std::string &path) {}

  virtual void before_run();

  virtual void after_run();

  virtual void begin_time_step();

  virtual void monthly_report();

 private:

};

#endif    /* CONSOLEREPORTER_H */

