/*
 * File:   PkPdReporter.cpp
 * Author: Merlin
 *
 * Created on October 29, 2014, 12:56 PM
 */

#include "PkPdReporter.h"

#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Utils/Index/PersonIndexAll.h"

PkPdReporter::PkPdReporter(utils::DxGAppInput* app_input) : app_input_{app_input} {}

PkPdReporter::~PkPdReporter() {
  if (parasitaemia_file_.is_open()) { parasitaemia_file_.close(); }
}

void PkPdReporter::initialize(int /*job_number*/, const std::string &path) { prefix_ = path; }

void PkPdReporter::before_run() {
  // Open CSV output — use appInput->output_file if provided, else fall back to prefix
  std::string out_path;
  if ((app_input_ != nullptr) && !app_input_->output_file.empty()) {
    out_path = app_input_->output_file;
  } else {
    out_path = fmt::format("{}_parasitaemia.csv", prefix_);
  }
  parasitaemia_file_.open(out_path, std::ios::out);
  parasitaemia_file_ << "time,individual,recrudescence,parasitaemia" << '\n';
}

void PkPdReporter::begin_time_step() {
  Model::get_mdc()->perform_population_statistic();

  // Fake PfPR for recrudescence study
  Model::get_mdc()->blood_slide_prevalence_by_location()[0] = 0.1;

  if ((Model::get_scheduler()->current_time()
           % Model::get_config()->get_model_settings().get_days_between_stdout_output()
       == 0)
      && (Model::get_scheduler()->current_time()
          >= Model::get_config()->get_simulation_timeframe().get_start_collect_data_day())) {
    auto current_time = Model::get_scheduler()->current_time();

    for (int i = 0; i < Model::get_population()->all_persons()->v_person().size(); i++) {
      auto* person = Model::get_population()->all_persons()->v_person()[i].get();

      auto recrudescence_state = person->get_recurrence_status();

      double parasitaemia = 0.0;
      if (person->get_all_clonal_parasite_populations()->size() >= 1) {
        parasitaemia = person->get_all_clonal_parasite_populations()
                           ->at(0)
                           ->last_update_log10_parasite_density();
      } else {
        parasitaemia = Model::get_config()
                           ->get_parasite_parameters()
                           .get_parasite_density_levels()
                           .get_log_parasite_density_cured();
      }

      parasitaemia_file_ << current_time << "," << i << "," << static_cast<int>(recrudescence_state)
                         << "," << parasitaemia << '\n';
    }
  }
}

void PkPdReporter::after_time_step() {}

void PkPdReporter::monthly_report() {}

void PkPdReporter::after_run() {
  Model::get_mdc()->update_after_run();
  parasitaemia_file_.close();
}
