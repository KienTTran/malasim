#include "Reporter.h"
// #include "ConsoleReporter.h"
#include "Simulation/Model.h"
#include "MonthlyReporter.h"
#include "ValidationReporter.h"
// #include "MMCReporter.h"
// #include "TACTReporter.h"
// #include "NovelDrugReporter.h"
// #include "ValidationReporter.h"

std::map<std::string, Reporter::ReportType> Reporter::ReportTypeMap{
    {"Console",         CONSOLE},
    {"MonthlyReporter", MONTHLY_REPORTER},
    {"MMC",             MMC_REPORTER},
    {"TACT",            TACT_REPORTER},
    {"NovelDrug",       NOVEL_DRUG_REPOTER},
    {"ValidationReporter",       VALIDATION_REPORTER},
};

Reporter::Reporter() : model_(nullptr) {
}

Reporter::~Reporter() = default;

Reporter* Reporter::MakeReport(ReportType report_type) {
  switch (report_type) {
  //   case CONSOLE:
  //     return new ConsoleReporter();
  case MONTHLY_REPORTER:
    return new MonthlyReporter();
  //   case MMC_REPORTER:
  //     return new MMCReporter();
  //   case TACT_REPORTER:
  //     return new TACTReporter();
  //   case NOVEL_DRUG_REPOTER:
  //     return new NovelDrugReporter();
  case VALIDATION_REPORTER:
    return new ValidationReporter();
  default:
  return new MonthlyReporter();
  }
}