#include "SQLiteMonthlyReporter.h"

#include <fmt/printf.h>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Parasites/Genotype.h"
#include "Population/Population.h"
#include "Simulation//Model.h"
#include "Spatial/GIS/SpatialData.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"

// Initialize the reporter
// Sets up the database and prepares it for data entry
void SQLiteMonthlyReporter::initialize(int jobNumber, const std::string &path) {
  int admin_level_count =
      Model::get_spatial_data()->get_admin_level_manager()->get_level_names().size();

  if (admin_level_count == 0) {
    spdlog::info("No admin levels found, cell level reporting will be enabled.");
    enable_cell_level_reporting = true;
  }

  // Inform the user of the reporter type
  if (enable_cell_level_reporting) {
    spdlog::info(
        "Using SQLiteMonthlyReporter with aggregation at cell/pixel level AND multiple admin "
        "levels.");
  } else {
    spdlog::info(
        "Using SQLiteMonthlyReporter with aggregation at multiple admin levels, cell level "
        "reporting is disabled.");
  }

  SQLiteDbReporter::initialize(jobNumber, path);

  // Add +1 for cell level
  monthly_site_data_by_level.resize(admin_level_count + 1);
  monthly_genome_data_by_level.resize(admin_level_count + 1);
  CELL_LEVEL_ID = admin_level_count;
}

void SQLiteMonthlyReporter::count_infections_for_location(int level_id, int location_id) {
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);
  std::vector<int> ageClasses = Model::get_config()->age_structure();
  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    for (unsigned int ac = 0; ac < ageClasses.size(); ac++) {
      for (auto &person : index->vPerson()[location_id][hs][ac]) {
        // Is the individual infected by at least one parasite?
        if (person->get_all_clonal_parasite_populations()->empty()) { continue; }

        monthly_site_data_by_level[level_id].infections_by_unit[unit_id]++;
      }
    }
  }
}

void SQLiteMonthlyReporter::calculate_and_build_up_site_data_insert_values(int monthId,
                                                                           int level_id) {
  int min_unit_id = 0;
  int max_unit_id = 0;
  if (level_id == CELL_LEVEL_ID) {
    min_unit_id = 0;
    max_unit_id = Model::get_config()->number_of_locations() - 1;
  } else {
    // Get the boundary for this admin level
    const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
        Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);

    min_unit_id = boundary->min_unit_id;
    max_unit_id = boundary->max_unit_id;
  }

  insert_values.clear();

  for (auto unit_id = min_unit_id; unit_id <= max_unit_id; unit_id++) {
    // Skip units with no population
    if (monthly_site_data_by_level[level_id].population[unit_id] == 0) continue;

    double calculatedEir = (monthly_site_data_by_level[level_id].eir[unit_id] != 0)
                               ? (monthly_site_data_by_level[level_id].eir[unit_id]
                                  / monthly_site_data_by_level[level_id].population[unit_id])
                               : 0;
    double calculatedPfprUnder5 = (monthly_site_data_by_level[level_id].pfpr_under5[unit_id] != 0)
                                      ? (monthly_site_data_by_level[level_id].pfpr_under5[unit_id]
                                         / monthly_site_data_by_level[level_id].population[unit_id])
                                            * 100.0
                                      : 0;
    double calculatedPfpr2to10 = (monthly_site_data_by_level[level_id].pfpr2to10[unit_id] != 0)
                                     ? (monthly_site_data_by_level[level_id].pfpr2to10[unit_id]
                                        / monthly_site_data_by_level[level_id].population[unit_id])
                                           * 100.0
                                     : 0;
    double calculatedPfprAll = (monthly_site_data_by_level[level_id].pfpr_all[unit_id] != 0)
                                   ? (monthly_site_data_by_level[level_id].pfpr_all[unit_id]
                                      / monthly_site_data_by_level[level_id].population[unit_id])
                                         * 100.0
                                   : 0;

    std::string singleRow =
        fmt::format("({}, {}, {}, {}", monthId, unit_id,
                    monthly_site_data_by_level[level_id].population[unit_id],
                    monthly_site_data_by_level[level_id].clinical_episodes[unit_id]);

    // Append clinical episodes by age class
    for (const auto &episodes :
         monthly_site_data_by_level[level_id].clinical_episodes_by_age_class[unit_id]) {
      singleRow += fmt::format(", {}", episodes);
    }

    singleRow +=
        fmt::format(", {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
                    monthly_site_data_by_level[level_id].treatments[unit_id], calculatedEir,
                    calculatedPfprUnder5, calculatedPfpr2to10, calculatedPfprAll,
                    monthly_site_data_by_level[level_id].infections_by_unit[unit_id],
                    monthly_site_data_by_level[level_id].treatment_failures[unit_id],
                    monthly_site_data_by_level[level_id].nontreatment[unit_id],
                    monthly_site_data_by_level[level_id].treatments_under5[unit_id],
                    monthly_site_data_by_level[level_id].treatments_over5[unit_id]);

    insert_values.push_back(singleRow);
  }
}

// Collect and store monthly site data
// Aggregates data related to various site metrics and stores them in the
// database
void SQLiteMonthlyReporter::monthly_report_site_data(int monthId) {
  TransactionGuard transaction{db.get()};

  // Handle all levels including cell level in a single loop
  int total_levels = monthly_site_data_by_level.size();

  for (int level_id = 0; level_id < total_levels; level_id++) {
    // level_id == -1 represents cell level
    bool is_cell_level = (level_id == CELL_LEVEL_ID);

    // Skip cell level if not enabled
    if (is_cell_level && !enable_cell_level_reporting) continue;

    int vector_size = -1;
    // Get vector size based on level
    if (is_cell_level) {
      vector_size = Model::get_config()->number_of_locations();
    } else {
      const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
          Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);
      vector_size = boundary->max_unit_id + 1;
    }

    std::vector<int> ageClasses = Model::get_config()->age_structure();
    // Reset data structures for this admin level
    reset_site_data_structures(level_id, vector_size, ageClasses.size());

    // Collect data for this admin level
    for (auto location = 0; location < Model::get_config()->number_of_locations(); location++) {
      auto locationPopulation = static_cast<int>(Model::get_population()->size(location));
      if (locationPopulation == 0) continue;

      collect_site_data_for_location(location, level_id);
    }

    // Calculate and insert data for this admin level
    calculate_and_build_up_site_data_insert_values(monthId, level_id);
    insert_monthly_site_data(level_id, insert_values);
  }
}

void SQLiteMonthlyReporter::collect_site_data_for_location(int location_id, int level_id) {
  // Get admin unit for this location at this level
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);

  std::vector<int> ageClasses = Model::get_config()->age_structure();

  count_infections_for_location(level_id, location_id);

  auto locationPopulation = static_cast<int>(Model::get_population()->size(location_id));
  // Collect the simple data
  monthly_site_data_by_level[level_id].population[unit_id] += static_cast<int>(locationPopulation);

  monthly_site_data_by_level[level_id].clinical_episodes[unit_id] +=
      Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[location_id];

  monthly_site_data_by_level[level_id].treatments[unit_id] +=
      Model::get_mdc()->monthly_number_of_treatment_by_location()[location_id];
  monthly_site_data_by_level[level_id].treatment_failures[unit_id] +=
      Model::get_mdc()->monthly_treatment_failure_by_location()[location_id];
  monthly_site_data_by_level[level_id].nontreatment[unit_id] +=
      Model::get_mdc()->monthly_nontreatment_by_location()[location_id];

  for (auto ndx = 0; ndx < ageClasses.size(); ndx++) {
    // Collect the treatment by age class, following the 0-59 month convention
    // for under-5
    if (ageClasses[ndx] < 5) {
      monthly_site_data_by_level[level_id].treatments_under5[unit_id] +=
          Model::get_mdc()->monthly_number_of_treatment_by_location_age_class()[location_id][ndx];
    } else {
      monthly_site_data_by_level[level_id].treatments_over5[unit_id] +=
          Model::get_mdc()->monthly_number_of_treatment_by_location_age_class()[location_id][ndx];
    }

    // collect the clinical episodes by age class
    monthly_site_data_by_level[level_id].clinical_episodes_by_age_class[unit_id][ndx] +=
        Model::get_mdc()
            ->monthly_number_of_clinical_episode_by_location_age_class()[location_id][ndx];
  }

  // EIR and PfPR is a bit more complicated since it could be an invalid value
  // early in the simulation, and when aggregating at the district level the
  // weighted mean needs to be reported instead
  if (Model::get_mdc()->recording_data()) {
    auto eirLocation = Model::get_mdc()->eir_by_location_year()[location_id].empty()
                           ? 0
                           : Model::get_mdc()->eir_by_location_year()[location_id].back();
    monthly_site_data_by_level[level_id].eir[unit_id] += (eirLocation * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr_under5[unit_id] +=
        (Model::get_mdc()->get_blood_slide_prevalence(location_id, 0, 5) * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr2to10[unit_id] +=
        (Model::get_mdc()->get_blood_slide_prevalence(location_id, 2, 10) * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr_all[unit_id] +=
        (Model::get_mdc()->blood_slide_prevalence_by_location()[location_id] * locationPopulation);
  }
}

void SQLiteMonthlyReporter::collect_genome_data_for_location(size_t location_id, int level_id) {
  auto unit_id = (level_id == CELL_LEVEL_ID)
                     ? location_id
                     : Model::get_spatial_data()->get_admin_unit(level_id, location_id);
  auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto ageClasses = index->vPerson()[0][0].size();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    // Iterate over all the age classes
    for (unsigned int ac = 0; ac < ageClasses; ac++) {
      // Iterate over all the genotypes
      auto peopleInAgeClass = index->vPerson()[location_id][hs][ac];
      for (auto &person : peopleInAgeClass) {
        collect_genome_data_for_a_person(person, unit_id, level_id);
      }
    }
  }
}

void SQLiteMonthlyReporter::reset_site_data_structures(int level_id, int vector_size,
                                                       size_t numAgeClasses) {
  // reset the data structures
  monthly_site_data_by_level[level_id].eir.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr_under5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr2to10.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].pfpr_all.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].population.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].clinical_episodes.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].clinical_episodes_by_age_class.assign(
      vector_size, std::vector<int>(numAgeClasses, 0));
  monthly_site_data_by_level[level_id].treatments.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatment_failures.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].nontreatment.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatments_under5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].treatments_over5.assign(vector_size, 0);
  monthly_site_data_by_level[level_id].infections_by_unit.assign(vector_size, 0);
}

void SQLiteMonthlyReporter::reset_genome_data_structures(int level_id, int vector_size,
                                                         size_t numGenotypes) {
  // reset the data structures
  monthly_genome_data_by_level[level_id].occurrences.assign(vector_size,
                                                            std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].clinical_occurrences.assign(
      vector_size, std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_0_5.assign(vector_size,
                                                                std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_2_10.assign(vector_size,
                                                                 std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].weighted_occurrences.assign(
      vector_size, std::vector<double>(numGenotypes, 0));
}

void SQLiteMonthlyReporter::collect_genome_data_for_a_person(Person* person, int unit_id,
                                                             int level_id) {
  const auto numGenotypes = Model::get_config()->number_of_parasite_types();
  auto individual = std::vector<int>(numGenotypes, 0);
  // Get the person, press on if they are not infected
  auto &parasites = *person->get_all_clonal_parasite_populations();
  auto numClones = parasites.size();
  if (numClones == 0) { return; }

  // Note the age and clinical status of the person
  auto age = person->get_age();
  auto clinical = static_cast<int>(person->get_host_state() == Person::HostStates::CLINICAL);

  // Count the genotypes present in the individual
  for (unsigned int ndx = 0; ndx < numClones; ndx++) {
    auto* parasitePopulation = parasites[ndx];
    auto genotypeId = parasitePopulation->genotype()->genotype_id();
    monthly_genome_data_by_level[level_id].occurrences[unit_id][genotypeId]++;
    monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotypeId] +=
        (age <= 5) ? 1 : 0;
    monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotypeId] +=
        (age >= 2 && age <= 10) ? 1 : 0;
    individual[genotypeId]++;

    // Count a clinical occurrence if the individual has clinical
    // symptoms
    monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotypeId] += clinical;
  }

  // Update the weighted occurrences and reset the individual count
  for (unsigned int ndx = 0; ndx < numGenotypes; ndx++) {
    if (individual[ndx] == 0) { continue; }
    monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][ndx] +=
        (individual[ndx] / static_cast<double>(numClones));
  }
}

void SQLiteMonthlyReporter::build_up_genome_data_insert_values(int monthId, int level_id) {
  auto numGenotypes = Model::get_config()->number_of_parasite_types();

  int min_unit_id = 0;
  int max_unit_id = 0;
  if (level_id == CELL_LEVEL_ID) {
    min_unit_id = 0;
    max_unit_id = Model::get_config()->number_of_locations() - 1;
  } else {
    // Get the boundary for this admin level
    const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
        Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);
    min_unit_id = boundary->min_unit_id;
    max_unit_id = boundary->max_unit_id;
  }

  insert_values.clear();

  // Iterate over the admin units and append the query
  for (auto unit_id = min_unit_id; unit_id <= max_unit_id; unit_id++) {
    // Skip if there are no infections in this unit
    if (monthly_site_data_by_level[level_id].infections_by_unit[unit_id] == 0) { continue; }

    for (auto genotype = 0; genotype < numGenotypes; genotype++) {
      if (monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype] == 0) {
        continue;
      }
      std::string singleRow = fmt::format(
          "({}, {}, {}, {}, {}, {}, {}, {})", monthId, unit_id, genotype,
          monthly_genome_data_by_level[level_id].occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotype],
          monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype]);

      insert_values.push_back(singleRow);
    }
  }
}

void SQLiteMonthlyReporter::monthly_report_genome_data(int monthId) {
  TransactionGuard transaction{db.get()};

  // Get admin levels count
  int admin_level_count = monthly_site_data_by_level.size();

  // For each admin level
  for (int level_id = 0; level_id < admin_level_count; level_id++) {
    // level_id == -1 represents cell level
    bool is_cell_level = (level_id == CELL_LEVEL_ID);

    // Skip cell level if not enabled
    if (is_cell_level && !enable_cell_level_reporting) continue;

    int vector_size = -1;
    // Get vector size based on level
    if (is_cell_level) {
      vector_size = Model::get_config()->number_of_locations();
    } else {
      const auto* boundary = Model::get_spatial_data()->get_admin_level_manager()->get_boundary(
          Model::get_spatial_data()->get_admin_level_manager()->get_level_names()[level_id]);
      vector_size = boundary->max_unit_id + 1;
    }

    auto numGenotypes = Model::get_config()->number_of_parasite_types();

    reset_genome_data_structures(level_id, vector_size, numGenotypes);

    auto* index = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

    // Iterate over all locations
    for (auto location = 0; location < index->vPerson().size(); location++) {
      collect_genome_data_for_location(location, level_id);
    }

    build_up_genome_data_insert_values(monthId, level_id);

    if (insert_values.empty()) {
      spdlog::info(
          "No genotypes recorded in the simulation at timestep, "
          "{}",
          Model::get_scheduler()->current_time());
      continue;
    }

    insert_monthly_genome_data(level_id, insert_values);
  }
}
