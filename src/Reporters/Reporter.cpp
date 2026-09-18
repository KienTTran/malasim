#include "Reporter.h"

#include <spdlog/spdlog.h>

// #include "ConsoleReporter.h"
#include "Configuration/Config.h"
#include "ConsoleReporter.h"
#include "MMCReporter.h"
#include "MonthlyReporter.h"
#include "NovelDrugReporter.h"
#include "SQLiteMonthlyReporter.h"
#include "SQLiteValidationReporter.h"
#include "Simulation/Model.h"
#include "Specialist/AgeBandReporter.h"
#include "Specialist/CellularReporter.h"
#include "Specialist/PopulationReporter.h"
#include "Specialist/SeasonalImmunity.h"
#include "TACTReporter.h"
#ifdef ENABLE_TRAVEL_TRACKING
#include "TravelTrackingReporter.h"
#endif
#include "ValidationReporter.h"

std::map<std::string, Reporter::ReportType> Reporter::report_type_map = {
    {"Console", ReportType::CONSOLE},
    {"MonthlyReporter", ReportType::MONTHLY_REPORTER},
    {"MMC", ReportType::MMC_REPORTER},
    {"TACT", ReportType::TACT_REPORTER},
    {"NovelDrug", ReportType::NOVEL_DRUG_REPOTER},
    {"ValidationReporter", ReportType::VALIDATION_REPORTER},
    {"PopulationReporter", ReportType::POPULATION_REPORTER},
    {"CellularReporter", ReportType::CELLULAR_REPORTER},
    {"SeasonalImmunity", ReportType::SEASONAL_IMMUNITY},
    {"AgeBand", ReportType::AGE_BAND_REPORTER},
    {"SQLiteMonthlyReporter", ReportType::SQLITE_MONTHLY_REPORTER},
    {"SQLiteValidationReporter", ReportType::SQLITE_VALIDATION_REPORTER},
#ifdef ENABLE_TRAVEL_TRACKING
    {"TravelTrackingReporter", ReportType::TRAVEL_TRACKING_REPORTER},
#endif
};

Reporter::Reporter() : model_(nullptr) {}

Reporter::~Reporter() = default;

std::unique_ptr<Reporter> Reporter::make_report(ReportType report_type) {
  switch (report_type) {
    case ReportType::CONSOLE:
      return std::make_unique<ConsoleReporter>();
    case ReportType::MONTHLY_REPORTER:
      return std::make_unique<MonthlyReporter>();
    case ReportType::MMC_REPORTER:
      return std::make_unique<MMCReporter>();
    case ReportType::TACT_REPORTER:
      return std::make_unique<TACTReporter>();
    case ReportType::NOVEL_DRUG_REPOTER:
      return std::make_unique<NovelDrugReporter>();
    case ReportType::VALIDATION_REPORTER:
      return std::make_unique<ValidationReporter>();
    case ReportType::POPULATION_REPORTER:
      return std::make_unique<PopulationReporter>();
    case ReportType::CELLULAR_REPORTER:
      return std::make_unique<CellularReporter>();
    case ReportType::SEASONAL_IMMUNITY:
      return std::make_unique<SeasonalImmunity>();
    case ReportType::AGE_BAND_REPORTER:
      return std::make_unique<AgeBandReporter>();
    case ReportType::SQLITE_MONTHLY_REPORTER: {
      auto cell_level_reporting =
          Model::get_config()->get_model_settings().get_cell_level_reporting();
      return std::make_unique<SQLiteMonthlyReporter>(cell_level_reporting);
    }
    case ReportType::SQLITE_VALIDATION_REPORTER:
      // NOTE: unlike SQLiteMonthlyReporter, this reporter does not take a
      // cell_level_reporting argument; it always writes at its own resolution.
      return std::make_unique<SQLiteValidationReporter>();
#ifdef ENABLE_TRAVEL_TRACKING
    case ReportType::TRAVEL_TRACKING_REPORTER:
      return std::make_unique<TravelTrackingReporter>();
#endif

    // Declared in the enum but not implemented in v6. Previously these fell
    // through to `default:` and silently produced a MonthlyReporter, which made
    // a missing implementation look like a working run with the wrong output.
    case ReportType::MOVEMENT_REPORTER:
      spdlog::error("MovementReporter is not implemented in this build.");
      return nullptr;
    case ReportType::GENOTYPE_CARRIERS:
      spdlog::error("GenotypeCarriers reporter is not implemented in this build.");
      return nullptr;
    case ReportType::THERAPY_RECORD_REPORTER:
      spdlog::error("TherapyRecord reporter is not implemented in this build.");
      return nullptr;
  }

  spdlog::error("Unhandled reporter type: {}", static_cast<int>(report_type));
  return nullptr;
}

std::vector<std::string> Reporter::available_reporters() {
  std::vector<std::string> names;
  names.reserve(report_type_map.size());
  for (const auto &[name, type] : report_type_map) { names.push_back(name); }
  return names;
}
