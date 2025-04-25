/* 
 * File:   IndividualsFileReporter.cpp
 * Author: Merlin
 * 
 * Created on November 7, 2014, 11:06 AM
 */

#include "IndividualsFileReporter.h"
#include "Population/Population.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Simulation/Model.h"
#include "Utils/Index/PersonIndexAll.h"

IndividualsFileReporter::IndividualsFileReporter(const std::string &file_name) : file_name_(file_name) {
}

IndividualsFileReporter::~IndividualsFileReporter() {
}

void IndividualsFileReporter::before_run() {
  fs_.open(file_name_.c_str(), std::fstream::out);
}

void IndividualsFileReporter::begin_time_step() {

}

void IndividualsFileReporter::after_time_step() {
  if (Model::get_scheduler()->current_time()%Model::get_config()->get_model_settings().get_days_between_stdout_output()) {
    for (int i = 0; i < Model::get_population()->all_persons()->v_person().size(); i++) {
      Person *person = Model::get_population()->all_persons()->v_person()[i].get();
      double p_density = 0;
      if (person->get_all_clonal_parasite_populations()->size() >= 1) {
        p_density = person->get_all_clonal_parasite_populations()->at(
            0)->last_update_log10_parasite_density();
      } else {
        p_density = Model::get_config()->get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_cured();
      }
      fs_ << p_density << "\t";
    }
    fs_ << std::endl;
  }

}

void IndividualsFileReporter::monthly_report() {
}

void IndividualsFileReporter::after_run() {
  fs_.close();
}






