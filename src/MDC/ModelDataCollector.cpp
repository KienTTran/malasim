#include "ModelDataCollector.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Treatment/Therapies/SCTherapy.h"
#include "Utils/Constants.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include <spdlog/spdlog.h>

// Fill the vector indicated with zeros, this should compile into a memset call
// which is faster than a loop
#define zero_fill(vector) std::fill(vector.begin(), vector.end(), 0)
#define zero_fill_matrix_2d(vector) \
  std::for_each(vector.begin(), vector.end(), [](auto &v) { zero_fill(v); })
#define zero_fill_matrix_3d(vector) \
  std::for_each(vector.begin(), vector.end(), [](auto &v) { zero_fill_matrix_2d(v); })

class PersonIndexByLocationStateAgeClass;

ModelDataCollector::ModelDataCollector(){}

ModelDataCollector::~ModelDataCollector() = default;

void ModelDataCollector::initialize() {
    births_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);
    deaths_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);
    malaria_deaths_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);
    malaria_deaths_by_location_age_class_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));

    popsize_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);
    popsize_residence_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);

    blood_slide_prevalence_by_location_ =
        DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    blood_slide_prevalence_by_location_age_group_ =
        DoubleVector2(Model::get_config()->number_of_locations(),
                      DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    blood_slide_number_by_location_age_group_ =
        DoubleVector2(Model::get_config()->number_of_locations(),
                      DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    blood_slide_prevalence_by_location_age_group_by_5_ =
        DoubleVector2(Model::get_config()->number_of_locations(),
                      DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    blood_slide_number_by_location_age_group_by_5_ =
        DoubleVector2(Model::get_config()->number_of_locations(),
                      DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    blood_slide_prevalence_by_location_age_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(80, 0.0));
    blood_slide_number_by_location_age_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(80, 0.0));
    fraction_of_positive_that_are_clinical_by_location_ =
        DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    popsize_by_location_hoststate_ = IntVector2(Model::get_config()->number_of_locations(),
                                                IntVector(Person::NUMBER_OF_STATE, 0));
    popsize_by_location_age_class_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    popsize_by_location_age_class_by_5_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    popsize_by_location_hoststate_age_class_ =
        IntVector3(Model::get_config()->number_of_locations(),
                   IntVector2(Person::NUMBER_OF_STATE,
                              IntVector(Model::get_config()->number_of_age_classes(), 0)));

    total_immune_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    total_immune_by_location_age_class_ =
        DoubleVector2(Model::get_config()->number_of_locations(),
                      DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    total_immune_by_location_age_ =
        DoubleVector2(Model::get_config()->number_of_locations(),
                      DoubleVector(100, 0.0));

    // Effective immunity / learned boost aggregates
    total_effective_boost_immune_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    total_effective_boost_immune_by_location_age_class_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    total_effective_boost_immune_by_location_age_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(100, 0.0));

    total_effective_clinical_immune_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    total_effective_clinical_immune_by_location_age_class_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    total_effective_clinical_immune_by_location_age_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(100, 0.0));

    total_effective_clearance_immune_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    total_effective_clearance_immune_by_location_age_class_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(Model::get_config()->number_of_age_classes(), 0.0));
    total_effective_clearance_immune_by_location_age_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(100, 0.0));

    total_number_of_bites_by_location_ = LongVector(Model::get_config()->number_of_locations(), 0);
    total_number_of_bites_by_location_year_ =
        LongVector(Model::get_config()->number_of_locations(), 0);
    person_days_by_location_year_ = LongVector(Model::get_config()->number_of_locations(), 0);

    eir_by_location_year_ =
        DoubleVector2(Model::get_config()->number_of_locations());
    eir_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);

    cumulative_clinical_episodes_by_location_ =
        LongVector(Model::get_config()->number_of_locations(), 0);
    cumulative_clinical_episodes_by_location_age_ =
        LongVector2(Model::get_config()->number_of_locations(), LongVector(100, 0));
    cumulative_clinical_episodes_by_location_age_group_ =
        LongVector2(Model::get_config()->number_of_locations(),
                    LongVector(Model::get_config()->number_of_age_classes(), 0));

    average_number_biten_by_location_person_ =
        DoubleVector2(Model::get_config()->number_of_locations());
    percentage_bites_on_top_20_by_location_ =
        DoubleVector(Model::get_config()->number_of_locations(), 0.0);

    cumulative_mutants_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);

    cumulative_discounted_ntf_by_location_ =
        DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    cumulative_ntf_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    cumulative_tf_by_location_ = LongVector(Model::get_config()->number_of_locations(), 0.0);
    cumulative_number_treatments_by_location_ =
        LongVector(Model::get_config()->number_of_locations(), 0.0);

    today_tf_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);
    today_number_of_treatments_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);
    today_ritf_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);

    total_number_of_treatments_60_by_location_ = IntVector2(
        Model::get_config()->number_of_locations(),
        IntVector(Model::get_config()->get_epidemiological_parameters().get_tf_window_size(), 0));
    total_ritf_60_by_location_ = IntVector2(
        Model::get_config()->number_of_locations(),
        IntVector(Model::get_config()->get_epidemiological_parameters().get_tf_window_size(), 0));
    total_tf_60_by_location_ = IntVector2(
        Model::get_config()->number_of_locations(),
        IntVector(Model::get_config()->get_epidemiological_parameters().get_tf_window_size(), 0));

    current_ritf_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    current_tf_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);

    progress_to_clinical_in_7d_counter = std::vector<ProgressToClinicalCounter>(
        Model::get_config()->number_of_locations(),ProgressToClinicalCounter());

    current_utl_duration_ = 0;
    utl_duration_ = IntVector();

    number_of_treatments_with_therapy_id_ = IntVector(Model::get_therapy_db().size(), 0);
    number_of_treatments_success_with_therapy_id_ = IntVector(Model::get_therapy_db().size(), 0);
    number_of_treatments_fail_with_therapy_id_ = IntVector(Model::get_therapy_db().size(), 0);

    mosquito_recombination_events_count_ =
        LongVector2(Model::get_config()->number_of_locations(), LongVector(2, 0));

    amu_per_parasite_pop_ = 0;
    amu_per_person_ = 0;
    amu_for_clinical_caused_parasite_ = 0;

    afu_ = 0;

    discounted_amu_per_parasite_pop_ = 0;
    discounted_amu_per_person_ = 0;
    discounted_amu_for_clinical_caused_parasite_ = 0;

    discounted_afu_ = 0;

    multiple_of_infection_by_location_ = IntVector2(Model::get_config()->number_of_locations(),
                                                    IntVector(NUMBER_OF_REPORTED_MOI, 0));

    current_eir_by_location_ = DoubleVector(Model::get_config()->number_of_locations(), 0.0);
    last_update_total_number_of_bites_by_location_ =
        LongVector(Model::get_config()->number_of_locations(), 0);

    last_10_blood_slide_prevalence_by_location_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(10, 0.0));
    last_10_fraction_positive_that_are_clinical_by_location_ =
        DoubleVector2(Model::get_config()->number_of_locations(), DoubleVector(10, 0.0));
    last_10_fraction_positive_that_are_clinical_by_location_age_class_ = DoubleVector3(
        Model::get_config()->number_of_locations(),
        DoubleVector2(Model::get_config()->number_of_age_classes(), DoubleVector(10, 0.0)));
    last_10_fraction_positive_that_are_clinical_by_location_age_class_by_5_ = DoubleVector3(
        Model::get_config()->number_of_locations(),
        DoubleVector2(Model::get_config()->number_of_age_classes(), DoubleVector(10, 0.0)));
    total_parasite_population_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);
    number_of_positive_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);

    total_parasite_population_by_location_age_group_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    number_of_positive_by_location_age_group_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    number_of_clinical_by_location_age_group_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    number_of_clinical_by_location_age_group_by_5_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    number_of_death_by_location_age_group_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));

    number_of_untreated_cases_by_location_age_year_ =
        IntVector2(Model::get_config()->number_of_locations(), IntVector(80, 0));
    number_of_treatments_by_location_age_year_ =
        IntVector2(Model::get_config()->number_of_locations(), IntVector(80, 0));
    number_of_deaths_by_location_age_year_ =
        IntVector2(Model::get_config()->number_of_locations(), IntVector(80, 0));
    number_of_malaria_deaths_treated_by_location_age_year_ =
        IntVector2(Model::get_config()->number_of_locations(), IntVector(80, 0));
    number_of_malaria_deaths_non_treated_by_location_age_year_ =
        IntVector2(Model::get_config()->number_of_locations(), IntVector(80, 0));

    popsize_by_location_age_ =
        IntVector2(Model::get_config()->number_of_locations(), IntVector(80, 0));

    tf_at_15_ = 0;
    single_resistance_frequency_at_15_ = 0;
    double_resistance_frequency_at_15_ = 0;
    triple_resistance_frequency_at_15_ = 0;
    quadruple_resistance_frequency_at_15_ = 0;
    quintuple_resistance_frequency_at_15_ = 0;
    art_resistance_frequency_at_15_ = 0;
    total_resistance_frequency_at_15_ = 0;

    total_number_of_treatments_60_by_therapy_ = IntVector2(
        Model::get_therapy_db().size(),
        IntVector(Model::get_config()->get_epidemiological_parameters().get_tf_window_size(), 0));
    total_tf_60_by_therapy_ = IntVector2(
        Model::get_therapy_db().size(),
        IntVector(Model::get_config()->get_epidemiological_parameters().get_tf_window_size(), 0));
    current_tf_by_therapy_ = DoubleVector(Model::get_therapy_db().size(), 0.0);
    today_tf_by_therapy_ = IntVector(Model::get_therapy_db().size(), 0);
    today_number_of_treatments_by_therapy_ = IntVector(Model::get_therapy_db().size(), 0);

    monthly_number_of_treatment_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);
    monthly_number_of_recrudescence_treatment_by_location_ =
      LongVector(Model::get_config()->number_of_locations(), 0);
    monthly_number_of_recrudescence_treatment_by_location_age_class_ =
      LongVector2(Model::get_config()->number_of_locations(),
      LongVector(Model::get_config()->number_of_age_classes(), 0));
    monthly_number_of_recrudescence_treatment_by_location_age_ =
      LongVector2(Model::get_config()->number_of_locations(),
                 LongVector(80, 0));
    monthly_number_of_tf_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);
    monthly_number_of_new_infections_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);
    monthly_number_of_clinical_episode_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);
    monthly_number_of_clinical_episode_by_location_age_ =
        IntVector2(Model::get_config()->number_of_locations(), IntVector(100, 0));
    monthly_number_of_mutation_events_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);

    current_number_of_mutation_events_in_this_year_ = 0;
    mutation_tracker =
        std::vector<std::vector<MutationTrackerInfo>>(Model::get_config()->number_of_locations());

    mosquito_recombined_resistant_genotype_tracker =
        std::vector<std::vector<RecombinedResistantGenotypeInfo>>(
            Model::get_config()->number_of_locations());

    number_of_mutation_events_by_year_ = LongVector();

    monthly_treatment_failure_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);
    monthly_nontreatment_by_location_ = IntVector(Model::get_config()->number_of_locations(), 0);
    monthly_number_of_treatment_by_location_age_class_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    monthly_number_of_treatment_by_location_therapy_ = IntVector2(
        Model::get_config()->number_of_locations(), IntVector(Model::get_therapy_db().size(), 0));
    monthly_number_of_clinical_episode_by_location_age_class_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    monthly_nontreatment_by_location_age_class_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    monthly_treatment_complete_by_location_therapy_ = IntVector2(
        Model::get_config()->number_of_locations(), IntVector(Model::get_therapy_db().size(), 0));
    monthly_treatment_failure_by_location_age_class_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    monthly_treatment_failure_by_location_therapy_ = IntVector2(
        Model::get_config()->number_of_locations(), IntVector(Model::get_therapy_db().size(), 0));
    monthly_treatment_success_by_location_ =
        IntVector(Model::get_config()->number_of_locations(), 0);
    monthly_treatment_success_by_location_age_class_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector(Model::get_config()->number_of_age_classes(), 0));
    monthly_treatment_success_by_location_therapy_ = IntVector2(
        Model::get_config()->number_of_locations(), IntVector(Model::get_therapy_db().size(), 0));
    current_number_of_mutation_events_ = 0;
    number_of_mutation_events_by_year_ = LongVector();

    // Initialize new monthly_number_of_people_seeking_treatment_by_location_age_index_
    const auto ages_count = static_cast<int>(Model::get_config()->get_epidemiological_parameters().get_age_based_probability_of_seeking_treatment().get_ages().size());
    monthly_number_of_people_seeking_treatment_by_location_age_index_ =
        IntVector2(Model::get_config()->number_of_locations(),
                   IntVector((ages_count>0)?ages_count:1, 0));
}

void ModelDataCollector::perform_population_statistic() {
  // this will do every time the reporter execute the report
  zero_population_statistics();

  auto* pi = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();

  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        std::size_t size = pi->vPerson()[loc][hs][ac].size();
        popsize_by_location_hoststate_[loc][hs] += static_cast<int>(size);
        popsize_by_location_age_class_[loc][ac] += static_cast<int>(size);
        popsize_by_location_hoststate_age_class_[loc][hs][ac] += static_cast<int>(size);

        for (int i = 0; i < size; i++) {
          Person* person = pi->vPerson()[loc][hs][ac][i];
          popsize_residence_by_location_[person->get_residence_location()]++;

          //                    assert(p->has_birthday_event());
          //                    assert(p->get_age_class() == ac);
          // this immune value will include maternal immunity value of the infants
          double immune_value = person->get_immune_system()->get_latest_immune_value();
          total_immune_by_location_[loc] += immune_value;
          total_immune_by_location_age_class_[loc][ac] += immune_value;
          // Effective immunity (clinical and clearance) and total effective boost
          const double effective_clinical = person->get_immune_system()->get_effective_clinical_immunity(Model::get_scheduler()->current_time());
          const double effective_clearance = person->get_immune_system()->get_effective_clearance_immunity(Model::get_scheduler()->current_time());
          const double total_effective = effective_clinical + effective_clearance;
          total_effective_boost_immune_by_location_[loc] += total_effective;
          total_effective_boost_immune_by_location_age_class_[loc][ac] += total_effective;
           int age = static_cast<int>(person->get_age());
           int age_clamp = (age < 80) ? age : 79;
          total_immune_by_location_age_[loc][age_clamp] += immune_value;
          total_effective_boost_immune_by_location_age_[loc][age_clamp] += total_effective;
          // clinical and clearance specific aggregates
          total_effective_clinical_immune_by_location_[loc] += effective_clinical;
          total_effective_clinical_immune_by_location_age_class_[loc][ac] += effective_clinical;
          total_effective_clinical_immune_by_location_age_[loc][age_clamp] += effective_clinical;
          total_effective_clearance_immune_by_location_[loc] += effective_clearance;
          total_effective_clearance_immune_by_location_age_class_[loc][ac] += effective_clearance;
          total_effective_clearance_immune_by_location_age_[loc][age_clamp] += effective_clearance;
          //                    popsize_by_location_age_class_[loc][ac] += 1;
          int ac1 = (person->get_age() > 70) ? 14 : person->get_age() / 5;
          popsize_by_location_age_class_by_5_[loc][ac1] += 1;
          popsize_by_location_age_[loc][age_clamp] += 1;

          if (hs == Person::ASYMPTOMATIC) {
            number_of_positive_by_location_[loc]++;
            number_of_positive_by_location_age_group_[loc][ac] += 1;

            if (person->has_detectable_parasite()) {
              blood_slide_prevalence_by_location_[loc] += 1;
              blood_slide_number_by_location_age_group_[loc][ac] += 1;
              blood_slide_number_by_location_age_group_by_5_[loc][ac1] += 1;
              blood_slide_number_by_location_age_[loc][age_clamp] += 1;
            }
          } else if (hs == Person::CLINICAL) {
            number_of_positive_by_location_[loc]++;
            number_of_positive_by_location_age_group_[loc][ac] += 1;
            blood_slide_prevalence_by_location_[loc] += 1;
            blood_slide_number_by_location_age_group_[loc][ac] += 1;
            blood_slide_number_by_location_age_group_by_5_[loc][ac1] += 1;
            number_of_clinical_by_location_age_group_[loc][ac] += 1;
            number_of_clinical_by_location_age_group_by_5_[loc][ac1] += 1;
            blood_slide_number_by_location_age_[loc][age_clamp] += 1;
          }

          int moi = static_cast<int>(person->get_all_clonal_parasite_populations()->size());
          if (moi > 0) {
            total_parasite_population_by_location_[loc] += moi;
            total_parasite_population_by_location_age_group_[loc][person->get_age_class()] += moi;

              if (moi <= NUMBER_OF_REPORTED_MOI) {
                  multiple_of_infection_by_location_[loc][moi - 1]++;
              }
          }
        }
      }
    }

    fraction_of_positive_that_are_clinical_by_location_[loc] =
        (blood_slide_prevalence_by_location_[loc] == 0)
            ? 0
            : static_cast<double>(popsize_by_location_hoststate_[loc][Person::CLINICAL])
                  / blood_slide_prevalence_by_location_[loc];

    popsize_by_location_[loc] = Model::get_population()->size_at(static_cast<int>(loc));

    const auto number_of_blood_slide_positive = blood_slide_prevalence_by_location_[loc];
    blood_slide_prevalence_by_location_[loc] =
        blood_slide_prevalence_by_location_[loc] / static_cast<double>(popsize_by_location_[loc]);

    current_eir_by_location_[loc] =
        popsize_by_location_[loc] == 0
            ? 0
            : static_cast<double>(total_number_of_bites_by_location_[loc]
                                  - last_update_total_number_of_bites_by_location_[loc])
                  / static_cast<double>(popsize_by_location_[loc]);
    // spdlog::info("perform_population_statistic: location {} current_EIR_by_location_ {} =  {} -
    // {} / {}",
    //   loc,current_EIR_by_location_[loc],
    //   total_number_of_bites_by_location_[loc],
    //   last_update_total_number_of_bites_by_location_[loc],
    //   static_cast<double>(popsize_by_location_[loc]));
    last_update_total_number_of_bites_by_location_[loc] = total_number_of_bites_by_location_[loc];

    auto report_index =
        (Model::get_scheduler()->current_time()
         / Model::get_config()->get_model_settings().get_days_between_stdout_output())
        % 10;

    last_10_blood_slide_prevalence_by_location_[loc][report_index] =
        blood_slide_prevalence_by_location_[loc];
    last_10_fraction_positive_that_are_clinical_by_location_[loc][report_index] =
        fraction_of_positive_that_are_clinical_by_location_[loc];

    for (int ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
      last_10_fraction_positive_that_are_clinical_by_location_age_class_[loc][ac][report_index] =
          (blood_slide_prevalence_by_location_age_group_[loc][ac] == 0)
              ? 0
              : number_of_clinical_by_location_age_group_[loc][ac]
                    / static_cast<double>(blood_slide_prevalence_by_location_age_group_[loc][ac]);
      last_10_fraction_positive_that_are_clinical_by_location_age_class_by_5_
          [loc][ac][report_index] = (number_of_blood_slide_positive == 0)
                                        ? 0
                                        : number_of_clinical_by_location_age_group_by_5_[loc][ac]
                                              / number_of_blood_slide_positive;
      blood_slide_prevalence_by_location_age_group_[loc][ac] =
          blood_slide_number_by_location_age_group_[loc][ac]
          / static_cast<double>(popsize_by_location_age_class_[loc][ac]);

      blood_slide_prevalence_by_location_age_group_by_5_[loc][ac] =
          blood_slide_number_by_location_age_group_by_5_[loc][ac]
          / static_cast<double>(popsize_by_location_age_class_by_5_[loc][ac]);
    }
    for (int age = 0; age < 80; age++) {
      blood_slide_prevalence_by_location_age_[loc][age] =
          blood_slide_number_by_location_age_[loc][age]
          / static_cast<double>(popsize_by_location_age_[loc][age]);
    }
  }
}

void ModelDataCollector::collect_1_clinical_episode(const int &location, const int &age,
                                                    const int &age_class) {
  if (recording_) {
    monthly_number_of_clinical_episode_by_location_[location] += 1;

    if (age < 100) {
      monthly_number_of_clinical_episode_by_location_age_[location][age] += 1;
    } else {
      monthly_number_of_clinical_episode_by_location_age_[location][99] += 1;
    }
    monthly_number_of_clinical_episode_by_location_age_class_[location][age_class] += 1;
  }

  if (Model::get_scheduler()->current_time()
      >= Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period()) {
    cumulative_clinical_episodes_by_location_[location]++;
    if (age < 100) {
      cumulative_clinical_episodes_by_location_age_[location][age]++;
    } else {
      cumulative_clinical_episodes_by_location_age_[location][99]++;
    }
    cumulative_clinical_episodes_by_location_age_group_[location][age_class]++;
  }
}

void ModelDataCollector::record_1_death(const int &location,
                                        const int &birthday,
                                        const int &number_of_times_bitten,
                                        const int &age_group,
                                        const int &age) {
  if (Model::get_scheduler()->current_time()
      >= Model::get_config()
             ->get_simulation_timeframe()
             .get_start_collect_data_day()) {
      // spdlog::info("1 death");
      update_person_days_by_years(
          location,
          -(Constants::DAYS_IN_YEAR - Model::get_scheduler()->get_current_day_in_year()));
      update_average_number_bitten(location, birthday, number_of_times_bitten);
      number_of_death_by_location_age_group_[location][age_group] += 1;
      const int age_clamp = (age < 79) ? age : 79;
      number_of_deaths_by_location_age_year_[location][age_clamp] += 1;
      deaths_by_location_[location] += 1;
    }
}


void ModelDataCollector::record_1_malaria_death(const int &loc, const int &age, bool treated) {
  if (!recording_) return;
  const int age_clamp = (age < 80) ? age : 79;
  if (treated)  number_of_malaria_deaths_treated_by_location_age_year_[loc][age_clamp] += 1;
  else          number_of_malaria_deaths_non_treated_by_location_age_year_[loc][age_clamp] += 1;
}

void ModelDataCollector::update_average_number_bitten(const int &location, const int &birthday,
                                                      const int &number_of_times_bitten) {
  const auto time_living_from_start_collect_data_day =
      (birthday < Model::get_config()->get_simulation_timeframe().get_start_collect_data_day())
          ? 1
          : Model::get_scheduler()->current_time() + 1
                - Model::get_config()->get_simulation_timeframe().get_start_collect_data_day();
  const auto average_bites =
      number_of_times_bitten / static_cast<double>(time_living_from_start_collect_data_day);

  average_number_biten_by_location_person_[location].push_back(average_bites);
}

void ModelDataCollector::calculate_percentage_bites_on_top_20() {
  auto pi = Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < Model::get_config()->number_of_age_classes(); ac++) {
        for (auto p : pi->vPerson()[loc][hs][ac]) {
          // add to total average number bitten
          update_average_number_bitten(loc, p->get_birthday(), p->get_number_of_times_bitten());
        }
      }
    }
  }
  for (auto location = 0; location < Model::get_config()->number_of_locations(); location++) {
    std::sort(average_number_biten_by_location_person_[location].begin(),
              average_number_biten_by_location_person_[location].end(), std::greater<>());
    double total = 0;
    double t20 = 0;
    const auto size20 = static_cast<int>(
        std::round(average_number_biten_by_location_person_[location].size() / 100.0 * 20));

    for (auto i = 0;
         static_cast<size_t>(i) < average_number_biten_by_location_person_[location].size(); i++) {
      total += average_number_biten_by_location_person_[location][i];

      if (i <= size20) { t20 += average_number_biten_by_location_person_[location][i]; }
    }
    percentage_bites_on_top_20_by_location_[location] = (total == 0) ? 0 : t20 / total;
  }
}

void ModelDataCollector::record_1_non_treated_case(const int &location, const int &age,
                                                   const int &age_class) {
  if (recording_) {
    if (age <= 79) {
      number_of_untreated_cases_by_location_age_year_[location][age] += 1;
    } else {
      number_of_untreated_cases_by_location_age_year_[location][79] += 1;
    }
  }
  if (!recording_) { return; }
  monthly_nontreatment_by_location_[location]++;
  monthly_nontreatment_by_location_age_class_[location][age_class]++;
}

void ModelDataCollector::begin_time_step() {
  for (int location = 0; location < Model::get_config()->number_of_locations(); location++) {
    today_number_of_treatments_by_location_[location] = 0;
    today_ritf_by_location_[location] = 0;
    today_tf_by_location_[location] = 0;
  }

  for (auto therapy_id = 0; static_cast<size_t>(therapy_id) < Model::get_therapy_db().size();
       therapy_id++) {
    today_number_of_treatments_by_therapy_[therapy_id] = 0;
    today_tf_by_therapy_[therapy_id] = 0;
  }
  if (!recording_
      && Model::get_scheduler()->current_time()
             >= Model::get_config()->get_simulation_timeframe().get_start_collect_data_day()) {
    recording_ = true;
  }
}

// TODO: review
void ModelDataCollector::end_of_time_step() {
  if (!recording_) { return; }
  double avg_tf = 0;
  for (auto location = 0; location < Model::get_config()->number_of_locations(); location++) {
    total_number_of_treatments_60_by_location_
        [location][Model::get_scheduler()->current_time()
                   % Model::get_config()->get_epidemiological_parameters().get_tf_window_size()] =
            today_number_of_treatments_by_location_[location];
    total_ritf_60_by_location_
        [location][Model::get_scheduler()->current_time()
                   % Model::get_config()->get_epidemiological_parameters().get_tf_window_size()] =
            today_ritf_by_location_[location];
    total_tf_60_by_location_
        [location][Model::get_scheduler()->current_time()
                   % Model::get_config()->get_epidemiological_parameters().get_tf_window_size()] =
            today_tf_by_location_[location];

    auto t_treatment60 = 0;
    auto t_ritf60 = 0;
    auto t_tf60 = 0;
    for (auto i = 0;
         i < Model::get_config()->get_epidemiological_parameters().get_tf_window_size(); i++) {
      t_treatment60 += total_number_of_treatments_60_by_location_[location][i];
      t_ritf60 += total_ritf_60_by_location_[location][i];
      t_tf60 += total_tf_60_by_location_[location][i];
    }
    current_ritf_by_location_[location] =
        (t_treatment60 == 0) ? 0 : static_cast<double>(t_ritf60) / t_treatment60;
    current_tf_by_location_[location] =
        (t_treatment60 == 0) ? 0 : static_cast<double>(t_tf60) / t_treatment60;

    current_tf_by_location_[location] =
        (current_tf_by_location_[location]) > 1 ? 1 : current_tf_by_location_[location];
    current_ritf_by_location_[location] =
        (current_ritf_by_location_[location]) > 1 ? 1 : current_ritf_by_location_[location];

    avg_tf += current_tf_by_location_[location];
  }

  // update UTL
  if ((avg_tf / static_cast<double>(Model::get_config()->number_of_locations()))
      <= Model::get_config()->get_therapy_parameters().get_tf_rate()) {
    current_utl_duration_ += 1;
  }
  for (auto therapy_id = 0; static_cast<size_t>(therapy_id) < Model::get_therapy_db().size();
       therapy_id++) {
    total_number_of_treatments_60_by_therapy_[therapy_id][Model::get_scheduler()->current_time()
                                                          % Model::get_config()
                                                                ->get_epidemiological_parameters()
                                                                .get_tf_window_size()] =
        today_number_of_treatments_by_therapy_[therapy_id];
    total_tf_60_by_therapy_[therapy_id][Model::get_scheduler()->current_time()
                                        % Model::get_config()
                                              ->get_epidemiological_parameters()
                                              .get_tf_window_size()] =
        today_tf_by_therapy_[therapy_id];

    auto t_treatment60 = 0;
    auto t_tf60 = 0;
    for (auto i = 0;
         i < Model::get_config()->get_epidemiological_parameters().get_tf_window_size(); i++) {
      t_treatment60 += total_number_of_treatments_60_by_therapy_[therapy_id][i];
      t_tf60 += total_tf_60_by_therapy_[therapy_id][i];
    }

    current_tf_by_therapy_[therapy_id] =
        (t_treatment60 == 0) ? 0 : static_cast<double>(t_tf60) / t_treatment60;
    current_tf_by_therapy_[therapy_id] =
        (current_tf_by_therapy_[therapy_id]) > 1 ? 1 : current_tf_by_therapy_[therapy_id];
  }
}

void ModelDataCollector::record_1_treatment(const int &location, const int &age,
                                            const int &age_class, const int &therapy_id) {
  if (!recording_) { return; }
  today_number_of_treatments_by_location_[location] += 1;
  today_number_of_treatments_by_therapy_[therapy_id] += 1;
  number_of_treatments_with_therapy_id_[therapy_id] += 1;
  monthly_number_of_treatment_by_location_[location] += 1;
  if (age <= 79) {
    number_of_treatments_by_location_age_year_[location][age] += 1;
  } else {
    number_of_treatments_by_location_age_year_[location][79] += 1;
  }
  cumulative_number_treatments_by_location_[location] += 1;
  monthly_number_of_treatment_by_location_age_class_[location][age_class] += 1;
  monthly_number_of_treatment_by_location_therapy_[location][therapy_id] += 1;

  // Also count this as a person seeking treatment in the age-indexed monthly counter.
  // Compute age index using age breakpoints from config: find largest i such that ages[i] <= age.
  const auto &ages_vec = Model::get_config()->get_epidemiological_parameters().get_age_based_probability_of_seeking_treatment().get_ages();
  if (!monthly_number_of_people_seeking_treatment_by_location_age_index_.empty()) {
    int age_index = 0;
    if (!ages_vec.empty()) {
      // find largest index i where ages_vec[i] <= age
      age_index = 0;
      for (size_t i = 0; i < ages_vec.size(); ++i) {
        if (age >= ages_vec[i]) { age_index = static_cast<int>(i); }
        else { break; }
      }
      // ensure age_index within bounds
      if (age_index >= static_cast<int>(monthly_number_of_people_seeking_treatment_by_location_age_index_[location].size())) {
        age_index = static_cast<int>(monthly_number_of_people_seeking_treatment_by_location_age_index_[location].size()) - 1;
      }
    } else {
      age_index = 0;
    }
    if (age_index >= 0) {
      monthly_number_of_people_seeking_treatment_by_location_age_index_[location][age_index] += 1;
    }
  }
}

void ModelDataCollector::record_1_recrudescence_treatment(const int &location, const int &age,
                                                          const int &age_class,
                                                          const int &therapy_id) {
  if (!recording_) { return; }
  monthly_number_of_recrudescence_treatment_by_location_[location] += 1;
  monthly_number_of_recrudescence_treatment_by_location_age_class_[location][age_class] += 1;
  const int age_clamp = (age < 80) ? age : 79;
  monthly_number_of_recrudescence_treatment_by_location_age_[location][age_clamp] += 1;
}

void ModelDataCollector::record_1_mutation(const int &location, Genotype* from, Genotype* to) {
  if (!recording_) { return; }
  cumulative_mutants_by_location_[location] += 1;
  monthly_number_of_mutation_events_by_location_[location] += 1;
  current_number_of_mutation_events_in_this_year_ += 1;
  current_number_of_mutation_events_ += 1;
}

void ModelDataCollector::record_1_mutation_by_drug(const int &location, Genotype* from,
                                                   Genotype* to, int drug_id) {
  auto mutation_tracker_info = std::make_tuple(location, Model::get_scheduler()->current_time(),
                                               Model::get_scheduler()->get_current_month_in_year(),
                                               drug_id, from->genotype_id(), to->genotype_id());
  mutation_tracker[location].push_back(mutation_tracker_info);
}

void ModelDataCollector::record_1_treatment_failure_by_therapy(const int &location,
                                                               const int &age_class,
                                                               const int &therapy_id) {
  number_of_treatments_fail_with_therapy_id_[therapy_id] += 1;
  today_tf_by_therapy_[therapy_id] += 1;
  if (!recording_) { return; }
  monthly_treatment_complete_by_location_therapy_[location][therapy_id]++;
  monthly_treatment_failure_by_location_[location]++;
  monthly_treatment_failure_by_location_age_class_[location][age_class]++;
  monthly_treatment_failure_by_location_therapy_[location][therapy_id]++;
}

void ModelDataCollector::record_1_treatment_success_by_therapy(const int &location,
                                                               const int &age_class,
                                                               const int &therapy_id) {
  number_of_treatments_success_with_therapy_id_[therapy_id] += 1;
  if (!recording_) { return; }
  monthly_treatment_complete_by_location_therapy_[location][therapy_id]++;
  monthly_treatment_success_by_location_[location]++;
  monthly_treatment_success_by_location_age_class_[location][age_class]++;
  monthly_treatment_success_by_location_therapy_[location][therapy_id]++;
}

void ModelDataCollector::update_utl_vector() {
  utl_duration_.push_back(current_utl_duration_);
  current_utl_duration_ = 0;
}

void ModelDataCollector::record_1_tf(const int &location, const bool &by_drug) {
  // if treatment failure due to drug (both resistance or not clear)
  if (by_drug && recording_) {
    today_tf_by_location_[location] += 1;
    monthly_number_of_tf_by_location_[location] += 1;
  }

  if (Model::get_scheduler()->current_time()
      >= Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period()) {
    const auto current_discounted_tf =
        exp(log(0.97)
            * floor((Model::get_scheduler()->current_time()
                     - Model::get_config()->get_simulation_timeframe().get_start_collect_data_day())
                    / static_cast<double>(Constants::DAYS_IN_YEAR)));

    cumulative_discounted_ntf_by_location_[location] += current_discounted_tf;
    cumulative_ntf_by_location_[location] += 1;
    if (by_drug) { cumulative_tf_by_location_[location] += 1; }
  }
}

/*
 * From MainDataCollector
 */

void ModelDataCollector::collect_number_of_bites(const int &location,
                                                 const int &number_of_bites) {
  // v5 behaviour: start collecting once current_time >= start_collect_data_day
  if (Model::get_scheduler()->current_time()
      >= Model::get_config()
             ->get_simulation_timeframe()
             .get_start_collect_data_day()) {
    total_number_of_bites_by_location_[location]      += number_of_bites;
    total_number_of_bites_by_location_year_[location] += number_of_bites;
    // spdlog::info("ModelDataCollector::collect_number_of_bites {} {} {} {} {}",
    //   Model::get_scheduler()->current_time(),
    //   location, number_of_bites,
    // total_number_of_bites_by_location_[location],
    // total_number_of_bites_by_location_year_[location]);
             }
}



void ModelDataCollector::record_1_infection(const int &location) {
  if (!recording_) { return; }
  monthly_number_of_new_infections_by_location_[location]++;
}

void ModelDataCollector::yearly_update() {
  const auto current_time = Model::get_scheduler()->current_time();
  const auto start_collect =
      Model::get_config()
          ->get_simulation_timeframe()
          .get_start_collect_data_day();

  if (current_time == start_collect) {
    // First “year”: initialise person-days based on current population
    for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
      person_days_by_location_year_[loc] =
        Model::get_population()->size_at(loc) * Constants::DAYS_IN_YEAR;
      // spdlog::info("{} {} {} {} {}",current_time, loc, Model::get_population()->size_at(loc),
      //   Constants::DAYS_IN_YEAR,person_days_by_location_year_[loc]);
    }
  } else {
    // Subsequent years: compute EIR and reset yearly counters
    for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
      auto eir = (static_cast<double>(total_number_of_bites_by_location_year_[loc]) /
             static_cast<double>(person_days_by_location_year_[loc])) *
            Constants::DAYS_IN_YEAR;

      // v5: always push yearly EIR (the “only record year have positive EIR”
      // line was commented out there as well)
      eir_by_location_year_[loc].push_back(eir);

      // Reset yearly accumulators
      person_days_by_location_year_[loc] =
        Model::get_population()->size_at(loc) * Constants::DAYS_IN_YEAR;
      // spdlog::info("{} {} {} {} {}",current_time, loc, Model::get_population()->size_at(loc),
      //   Constants::DAYS_IN_YEAR,person_days_by_location_year_[loc]);
      total_number_of_bites_by_location_year_[loc] = 0;

      for (auto age = 0; age < 80; age++) {
        number_of_untreated_cases_by_location_age_year_[loc][age]              = 0;
        number_of_treatments_by_location_age_year_[loc][age]                   = 0;
        number_of_deaths_by_location_age_year_[loc][age]                       = 0;
        number_of_malaria_deaths_treated_by_location_age_year_[loc][age]       = 0;
        number_of_malaria_deaths_non_treated_by_location_age_year_[loc][age]   = 0;
      }
    }

    // keep v5/v6 logic for mutation events as-is
    if (current_time >=
        Model::get_config()
            ->get_simulation_timeframe()
            .get_start_of_comparison_period()) {
      number_of_mutation_events_by_year_.push_back(
          current_number_of_mutation_events_in_this_year_);
      current_number_of_mutation_events_in_this_year_ = 0;
    }
  }
}
void ModelDataCollector::update_person_days_by_years(const int &location,
                                                     const int &days) {
  // v5 behaviour: only accumulate person-days once we start collecting
  if (Model::get_scheduler()->current_time()
      >= Model::get_config()
             ->get_simulation_timeframe()
             .get_start_collect_data_day()) {
    person_days_by_location_year_[location] += days;
    // spdlog::info("ModelDataCollector::update_person_days_by_years {} {} {} {}",Model::get_scheduler()->current_time(), location, days,person_days_by_location_year_[location]);
             }
}
void ModelDataCollector::calculate_eir() {
  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
    if (eir_by_location_year_[loc].empty()) {
      // collecting for less than 1 year since start_collect_data_day
      const auto start_collect =
          Model::get_config()
              ->get_simulation_timeframe()
              .get_start_collect_data_day();

      const double total_time_in_years =
          (Model::get_scheduler()->current_time() - start_collect) /
          static_cast<double>(Constants::DAYS_IN_YEAR);

      double eir = (person_days_by_location_year_[loc] == 0)
                       ? 0.0
                       : (static_cast<double>(
                              total_number_of_bites_by_location_year_[loc]) /
                          static_cast<double>(person_days_by_location_year_[loc])) *
                             Constants::DAYS_IN_YEAR;

      // average over the partial-year duration
      eir /= (total_time_in_years > 0.0 ? total_time_in_years : 1.0);
      eir_by_location_[loc] = eir;
    } else {
      // average over all non-zero yearly EIRs
      double sum_eir = std::accumulate(
          eir_by_location_year_[loc].begin(),
          eir_by_location_year_[loc].end(), 0.0);

      const auto number_of_0 = std::count(
          eir_by_location_year_[loc].begin(),
          eir_by_location_year_[loc].end(), 0.0);

      const double denom =
          static_cast<double>(eir_by_location_year_[loc].size() - number_of_0);

      eir_by_location_[loc] = (denom == 0.0) ? 0.0 : (sum_eir / denom);
    }
  }
}
void ModelDataCollector::update_after_run() {
  perform_population_statistic();

  calculate_eir();
  calculate_percentage_bites_on_top_20();

  utl_duration_.push_back(current_utl_duration_);

  current_utl_duration_ = 0;
  for (int utl : utl_duration_) { current_utl_duration_ += utl; }
}

void ModelDataCollector::record_amu_afu(Person* person, Therapy* therapy,
                                        ClonalParasitePopulation* clinical_caused_parasite) {
  if (Model::get_scheduler()->current_time()
      >= Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period()) {
    auto sc_therapy = dynamic_cast<SCTherapy*>(therapy);
    if (sc_therapy != nullptr) {
      const auto art_id = sc_therapy->get_artemisinin_id();
      if (art_id != -1 && sc_therapy->drug_ids.size() > 1) {
        const auto number_of_drugs_in_therapy = sc_therapy->drug_ids.size();
        const auto discounted_fraction = exp(
            log(0.97)
            * floor(
                (Model::get_scheduler()->current_time()
                 - Model::get_config()->get_simulation_timeframe().get_start_of_comparison_period())
                / Constants::DAYS_IN_YEAR));
        //            assert(false);
        // combine therapy
        for (auto i = 0; i < number_of_drugs_in_therapy; i++) {
          int drug_id = sc_therapy->drug_ids[i];
          if (drug_id != art_id) {
            // only check for the remaining chemical drug != artemisinin
            const auto parasite_population_size =
                person->get_all_clonal_parasite_populations()->size();

            auto found_amu = false;
            auto found_afu = false;
            for (auto j = 0; j < parasite_population_size; j++) {
              ClonalParasitePopulation* bp = person->get_all_clonal_parasite_populations()->at(j);
              if (bp->resist_to(drug_id) && !bp->resist_to(art_id)) {
                found_amu = true;
                amu_per_parasite_pop_ += sc_therapy->get_max_dosing_day()
                                         / static_cast<double>(parasite_population_size);
                discounted_amu_per_parasite_pop_ += discounted_fraction
                                                    * sc_therapy->get_max_dosing_day()
                                                    / static_cast<double>(parasite_population_size);
                if (bp == clinical_caused_parasite) {
                  amu_for_clinical_caused_parasite_ += sc_therapy->get_max_dosing_day();
                  discounted_amu_for_clinical_caused_parasite_ +=
                      discounted_fraction * sc_therapy->get_max_dosing_day();
                }
              }

              if (bp->resist_to(drug_id) && bp->resist_to(art_id)) { found_afu = true; }
            }
            if (found_amu) {
              amu_per_person_ += sc_therapy->get_max_dosing_day();
              discounted_amu_per_person_ += discounted_fraction * sc_therapy->get_max_dosing_day();
            }

            if (found_afu) {
              afu_ += sc_therapy->get_max_dosing_day();
              discounted_afu_ += discounted_fraction * sc_therapy->get_max_dosing_day();
            }
          }
        }
      }
    }
  }
}

double ModelDataCollector::get_blood_slide_prevalence(const int &location, const int &age_from,
                                                      const int &age_to) {
  double blood_slide_numbers = 0;
  double popsize = 0;
  //    age count from 0

  if (age_from < 10) {
    if (age_to <= 10) {
      for (int ac = age_from; ac <= age_to; ac++) {
        blood_slide_numbers += blood_slide_number_by_location_age_group_[location][ac];
        popsize += popsize_by_location_age_class_[location][ac];
      }
    } else {
      for (int ac = age_from; ac <= 10; ac++) {
        blood_slide_numbers += blood_slide_number_by_location_age_group_[location][ac];
        popsize += popsize_by_location_age_class_[location][ac];
      }
      int ac = 10;
      while (ac < age_to) {
        blood_slide_numbers += blood_slide_number_by_location_age_group_by_5_[location][ac / 5];
        popsize += popsize_by_location_age_class_by_5_[location][ac / 5];
        ac += 5;
      }
    }
  } else {
    int ac = age_from;

    while (ac < age_to) {
      blood_slide_numbers += blood_slide_number_by_location_age_group_by_5_[location][ac / 5];
      popsize += popsize_by_location_age_class_by_5_[location][ac / 5];
      ac += 5;
    }
  }
  return (popsize == 0) ? 0 : blood_slide_numbers / popsize;
}

void ModelDataCollector::monthly_update() {
  zero_fill(births_by_location_);
  zero_fill(deaths_by_location_);

  if (Model::get_scheduler()->current_time()
      >= Model::get_config()->get_simulation_timeframe().get_start_collect_data_day()) {
    zero_fill(monthly_number_of_treatment_by_location_);
    zero_fill(monthly_number_of_recrudescence_treatment_by_location_);
    zero_fill(monthly_number_of_new_infections_by_location_);
    zero_fill(monthly_number_of_clinical_episode_by_location_);
    zero_fill(monthly_nontreatment_by_location_);
    zero_fill(malaria_deaths_by_location_);
    zero_fill(monthly_treatment_failure_by_location_);
    zero_fill(monthly_treatment_success_by_location_);

    for (int loc = 0; loc < Model::get_config()->number_of_locations(); loc++) {
      zero_fill(monthly_nontreatment_by_location_age_class_[loc]);
      zero_fill(malaria_deaths_by_location_age_class_[loc]);
      zero_fill(monthly_number_of_clinical_episode_by_location_age_class_[loc]);
      zero_fill(monthly_number_of_treatment_by_location_therapy_[loc]);
      zero_fill(monthly_number_of_treatment_by_location_age_class_[loc]);
      zero_fill(monthly_treatment_complete_by_location_therapy_[loc]);
      zero_fill(monthly_treatment_failure_by_location_age_class_[loc]);
      zero_fill(monthly_treatment_failure_by_location_therapy_[loc]);
      zero_fill(monthly_treatment_success_by_location_age_class_[loc]);
      zero_fill(monthly_treatment_success_by_location_therapy_[loc]);
      zero_fill(monthly_number_of_recrudescence_treatment_by_location_age_class_[loc]);
      zero_fill(monthly_number_of_recrudescence_treatment_by_location_age_[loc]);

      monthly_number_of_tf_by_location_[loc] = 0;
      monthly_number_of_new_infections_by_location_[loc] = 0;
      monthly_number_of_clinical_episode_by_location_[loc] = 0;
      monthly_number_of_mutation_events_by_location_[loc] = 0;
      for (int age = 0; age < 100; age++) {
        monthly_number_of_clinical_episode_by_location_age_[loc][age] = 0;
      }
      // zero the new age-indexed seeking treatment counters
      if (!monthly_number_of_people_seeking_treatment_by_location_age_index_.empty()) {
        zero_fill(monthly_number_of_people_seeking_treatment_by_location_age_index_[loc]);
      }
    }
  }
}

void ModelDataCollector::zero_population_statistics() {
  // // Vectors to be zeroed
  // zero_fill(popsize_residence_by_location_);
  // zero_fill(blood_slide_prevalence_by_location_);
  // zero_fill(fraction_of_positive_that_are_clinical_by_location_);
  // zero_fill(total_immune_by_location_);
  // zero_fill(total_parasite_population_by_location_);
  // zero_fill(number_of_positive_by_location_);
  //
  // // Matrices based on number of locations to be zeroed
  // for (auto location = 0ul; location < Model::get_config()->number_of_locations();
  //      location++) {
  //   zero_fill(popsize_by_location_hoststate_[location]);
  //   zero_fill(total_immune_by_location_age_class_[location]);
  //   zero_fill(total_parasite_population_by_location_age_group_[location]);
  //   zero_fill(number_of_positive_by_location_age_group_[location]);
  //   zero_fill(number_of_clinical_by_location_age_group_[location]);
  //   zero_fill(number_of_clinical_by_location_age_group_by_5_[location]);
  //   zero_fill(popsize_by_location_age_class_[location]);
  //   zero_fill(popsize_by_location_age_class_by_5_[location]);
  //   zero_fill(blood_slide_prevalence_by_location_age_group_[location]);
  //   zero_fill(blood_slide_number_by_location_age_group_[location]);
  //   zero_fill(blood_slide_prevalence_by_location_age_group_by_5_[location]);
  //   zero_fill(blood_slide_number_by_location_age_group_by_5_[location]);
  //   zero_fill(multiple_of_infection_by_location_[location]);
  // }

  // Vectors to be zeroed
  zero_fill(popsize_by_location_);
  zero_fill(popsize_residence_by_location_);
  zero_fill(blood_slide_prevalence_by_location_);
  zero_fill(fraction_of_positive_that_are_clinical_by_location_);
  zero_fill(total_immune_by_location_);
  zero_fill(total_parasite_population_by_location_);
  zero_fill(number_of_positive_by_location_);

  zero_fill_matrix_2d(popsize_by_location_hoststate_);
  zero_fill_matrix_3d(popsize_by_location_hoststate_age_class_);
  zero_fill_matrix_2d(total_immune_by_location_age_class_);
  zero_fill_matrix_2d(total_immune_by_location_age_);
  // Zero effective immunity / learned boost aggregates
  zero_fill(total_effective_boost_immune_by_location_);
  zero_fill_matrix_2d(total_effective_boost_immune_by_location_age_class_);
  zero_fill_matrix_2d(total_effective_boost_immune_by_location_age_);
  zero_fill(total_effective_clinical_immune_by_location_);
  zero_fill_matrix_2d(total_effective_clinical_immune_by_location_age_class_);
  zero_fill_matrix_2d(total_effective_clinical_immune_by_location_age_);
  zero_fill(total_effective_clearance_immune_by_location_);
  zero_fill_matrix_2d(total_effective_clearance_immune_by_location_age_class_);
  zero_fill_matrix_2d(total_effective_clearance_immune_by_location_age_);
  zero_fill_matrix_2d(total_parasite_population_by_location_age_group_);
  zero_fill_matrix_2d(number_of_positive_by_location_age_group_);
  zero_fill_matrix_2d(number_of_clinical_by_location_age_group_);
  zero_fill_matrix_2d(number_of_clinical_by_location_age_group_by_5_);
  zero_fill_matrix_2d(popsize_by_location_age_class_);
  zero_fill_matrix_2d(popsize_by_location_age_class_by_5_);
  zero_fill_matrix_2d(blood_slide_prevalence_by_location_age_group_);
  zero_fill_matrix_2d(blood_slide_number_by_location_age_group_);
  zero_fill_matrix_2d(blood_slide_prevalence_by_location_age_group_by_5_);
  zero_fill_matrix_2d(blood_slide_number_by_location_age_group_by_5_);
  zero_fill_matrix_2d(blood_slide_prevalence_by_location_age_);
  zero_fill_matrix_2d(blood_slide_number_by_location_age_);
  zero_fill_matrix_2d(popsize_by_location_age_);
  zero_fill_matrix_2d(multiple_of_infection_by_location_);
}


void ModelDataCollector::record_1_person_seeking_treatment_by_location_age_index(const int &location, const int &age_index) {
  if (!recording_) { return; }
  if (location < 0 || location >= Model::get_config()->number_of_locations()) { return; }
  if (age_index < 0) { return; }
  const auto &ages_vec = Model::get_config()->get_epidemiological_parameters().get_age_based_probability_of_seeking_treatment().get_ages();
  const auto ages_count = static_cast<int>(ages_vec.size());
  const int ages_count_as_buckets = std::max(1, ages_count);
  int idx = age_index;
  if (idx >= ages_count_as_buckets) idx = ages_count_as_buckets - 1;
  monthly_number_of_people_seeking_treatment_by_location_age_index_[location][idx] += 1;
}





