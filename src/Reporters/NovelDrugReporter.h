#ifndef POMS_SRC_REPORTERS_NOVELDRUGREPORTER_H
#define POMS_SRC_REPORTERS_NOVELDRUGREPORTER_H

#include <sstream>
#include "Reporter.h"

class PersonIndexByLocationStateAgeClass;

class NovelDrugReporter : public Reporter {
public:
  //disallow copy, assign and move
  NovelDrugReporter(const NovelDrugReporter &) = delete;
  void operator=(const NovelDrugReporter &) = delete;
  NovelDrugReporter(NovelDrugReporter &&) = delete;
  NovelDrugReporter operator=(NovelDrugReporter &&) = delete;

public:

  NovelDrugReporter() = default;

  ~NovelDrugReporter() override = default;

  void initialize(int job_number, const std::string &path) override;

  void before_run() override;

  void after_run() override;

  void begin_time_step() override;

  void monthly_report() override;

private:
  void output_genotype_frequency_3(const int& number_of_genotypes, PersonIndexByLocationStateAgeClass* pi);

public:
  std::stringstream ss;
  const std::string group_sep = "-1111\t";
  const std::string sep = "\t";

  long cumulative_number_of_mutation_events_last_month = 0;

};


#endif //POMS_SRC_REPORTERS_NOVELDRUGREPORTER_H
