/*
 * AgeBandReporter.cpp
 *
 * Implementation of the AgeBandReporter class using SQLite.
 */
#include "AgeBandReporter.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Simulation/Model.h"

void AgeBandReporter::initialize(int job_number, const std::string &path) {
  // Setup spdlog multi-sink loggers
  auto pfpr_file = fmt::format("{}age_band_pfpr_{}.csv", path, job_number);
  auto cases_file = fmt::format("{}age_band_cases_{}.csv", path, job_number);

  auto pfpr_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(pfpr_file, true);
  auto pfpr_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  pfpr_console_sink->set_level(spdlog::get_level());
  pfpr_logger_ = std::make_shared<spdlog::logger>(
      "age_band_pfpr", spdlog::sinks_init_list{pfpr_file_sink, pfpr_console_sink});
  pfpr_logger_->set_pattern("%v");
  spdlog::register_logger(pfpr_logger_);

  auto cases_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(cases_file, true);
  auto cases_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  cases_console_sink->set_level(spdlog::get_level());
  cases_logger_ = std::make_shared<spdlog::logger>(
      "age_band_cases", spdlog::sinks_init_list{cases_file_sink, cases_console_sink});
  cases_logger_->set_pattern("%v");
  spdlog::register_logger(cases_logger_);

  // Determine when to start reporting
  start_recording_ = Model::get_config()->get_simulation_timeframe().get_total_time();
  start_recording_ -= 366;

  spdlog::info("Logging of age-banded blood slide prevalence will start at model day {}",
               start_recording_);

  // Build district lookup
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    district_lookup_.emplace_back(Model::get_spatial_data()->get_admin_unit("district", loc));
  }

  // Log headers
  pfpr_ << "ModelDays" << csv::SEP << "District" << csv::SEP;
  cases_ << "ModelDays" << csv::SEP << "District" << csv::SEP;
  for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
    pfpr_ << Model::get_config()->get_population_demographic().get_age_structure()[ac] << csv::SEP;
    auto band = Model::get_config()->get_population_demographic().get_age_structure()[ac];
    cases_ << "cases_" << band << csv::SEP << "pop_" << band << csv::SEP;
  }
  pfpr_ << csv::END_LINE;
  cases_ << csv::END_LINE;
  pfpr_logger_->info(pfpr_.str());
  cases_logger_->info(cases_.str());
  pfpr_.str("");
  cases_.str("");
}

void AgeBandReporter::monthly_report() {
  auto current_time = Model::get_scheduler()->current_time();
  if (current_time < start_recording_) { return; }

  auto age_classes = Model::get_config()->number_of_age_classes();
  auto districts = Model::get_spatial_data()->get_boundary("district")->unit_count;

  std::vector<std::vector<int>> population(districts, std::vector<int>(age_classes));
  std::vector<std::vector<double>> prevalence(districts, std::vector<double>(age_classes));

  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    auto district = Model::get_spatial_data()->get_boundary("district")->min_unit_id == 0
                        ? district_lookup_[loc]
                        : district_lookup_[loc] - 1;
    for (auto ac = 0; ac < age_classes; ac++) {
      population[district][ac] += Model::get_mdc()->popsize_by_location_age_class()[loc][ac];
      prevalence[district][ac] +=
          Model::get_mdc()->blood_slide_number_by_location_age_group()[loc][ac];
    }
  }

  for (auto district = 0; district < districts; district++) {
    pfpr_ << current_time << csv::SEP << district << csv::SEP;
    cases_ << current_time << csv::SEP << district << csv::SEP;
    for (auto ac = 0; ac < age_classes; ac++) {
      pfpr_ << ((prevalence[district][ac] != 0)
                    ? (prevalence[district][ac] / population[district][ac]) * 100.0
                    : 0)
            << csv::SEP;
      cases_ << prevalence[district][ac] << csv::SEP << population[district][ac] << csv::SEP;
    }
    pfpr_ << csv::END_LINE;
    cases_ << csv::END_LINE;
  }

  pfpr_logger_->info(pfpr_.str());
  cases_logger_->info(cases_.str());
  pfpr_.str("");
  cases_.str("");
}
