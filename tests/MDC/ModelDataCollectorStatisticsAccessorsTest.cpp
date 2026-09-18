#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <spdlog/spdlog.h>

#include "MDC/ModelDataCollector.h"
#include "Simulation/Model.h"
#include "Core/Scheduler/Scheduler.h"
#include "Population/Population.h"
#include "Configuration/Config.h"
#include "Population/Person/Person.h"
#include "Population/ClonalParasitePopulation.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "Parasites/Genotype.h"
#include "Treatment/Therapies/SCTherapy.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Constants.h"
#include "fixtures/TestFileGenerators.h"

using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;

/**
 * Test class for the ModelDataCollector
 * Tests the initialization, data collection, and statistics generation functionality
 */
class ModelDataCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment
        test_fixtures::setup_test_environment();
        
        // Set the input path to the config file
        utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
        
        // Initialize the model to load the config
        ASSERT_TRUE(Model::get_instance()->initialize());
        
        // Use the model's data collector, which should be initialized
        mdc_ = Model::get_mdc();
        
        // Initialize genotypes for testing
        setupGenotypes();
    }

    void TearDown() override {
        // Reset/release the model resources between tests
        Model::get_instance()->release();
        test_fixtures::cleanup_test_files();
    }
    
    // Helper method to set up initial state for tests
    void setupInitialState() {
        // Make sure the model data collector is initialized with proper values
        mdc_->initialize();

        // Setup for locations for testing
        const int num_locations = Model::get_config()->number_of_locations();
        for (int i = 0; i < num_locations; i++) {
            // Setup initial values for testing updates
            mdc_->today_number_of_treatments_by_location()[i] = 0;
            mdc_->today_tf_by_location()[i] = 0;
            mdc_->today_ritf_by_location()[i] = 0;
            mdc_->total_number_of_bites_by_location()[i] = 0;
            mdc_->total_number_of_bites_by_location_year()[i] = 0;
            mdc_->person_days_by_location_year()[i] = 0;
        }
    }

    void clearPersonIndex() {
        auto* person_index =
            Model::get_population()->get_person_index<PersonIndexByLocationStateAgeClass>();
        for (auto& location : person_index->vPerson()) {
            for (auto& state : location) {
                for (auto& age_class : state) {
                    age_class.clear();
                }
            }
        }
    }
    
    // Helper method to set up genotypes for testing
    void setupGenotypes() {
        // Create two genotypes with proper initialization as in PopulationGenerateIndividualTest
        auto from_genotype = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
        from_genotype->set_genotype_id(0); // Use a unique ID
        from_genotype_ptr_ = from_genotype.get();
        
        auto to_genotype = std::make_unique<Genotype>("||||YF2||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
        to_genotype->set_genotype_id(1); // Different ID
        to_genotype_ptr_ = to_genotype.get();
        
        // Add genotypes to the database
        Model::get_genotype_db()->add(std::move(from_genotype));
        Model::get_genotype_db()->add(std::move(to_genotype));
    }

    ModelDataCollector* mdc_ = nullptr;
    Genotype* from_genotype_ptr_ = nullptr;
    Genotype* to_genotype_ptr_ = nullptr;
};

// Test initialization of ModelDataCollector
TEST_F(ModelDataCollectorTest, InitializedStatisticsExposeConfiguredDimensions) {
    const auto location_count = Model::get_config()->number_of_locations();
    const auto age_class_count = Model::get_config()->number_of_age_classes();
    const auto therapy_count = Model::get_therapy_db().size();

#define EXPECT_LOCATION_ROWS(accessor) EXPECT_EQ(mdc_->accessor().size(), location_count)
#define EXPECT_AGE_ROWS(accessor) EXPECT_EQ(mdc_->accessor().size(), location_count)
#define EXPECT_THERAPY_ROWS(accessor) EXPECT_EQ(mdc_->accessor().size(), therapy_count)

    EXPECT_LOCATION_ROWS(total_immune_by_location);
    EXPECT_AGE_ROWS(total_immune_by_location_age_class);
    EXPECT_AGE_ROWS(total_immune_by_location_age);
    EXPECT_LOCATION_ROWS(popsize_by_location);
    EXPECT_LOCATION_ROWS(popsize_residence_by_location);
    EXPECT_AGE_ROWS(popsize_by_location_age_class);
    EXPECT_AGE_ROWS(popsize_by_location_age_class_by_5);
    EXPECT_AGE_ROWS(popsize_by_location_hoststate);
    EXPECT_AGE_ROWS(popsize_by_location_hoststate_age_class);
    EXPECT_LOCATION_ROWS(blood_slide_prevalence_by_location);
    EXPECT_AGE_ROWS(blood_slide_number_by_location_age_group);
    EXPECT_AGE_ROWS(blood_slide_prevalence_by_location_age_group);
    EXPECT_AGE_ROWS(blood_slide_number_by_location_age_group_by_5);
    EXPECT_AGE_ROWS(blood_slide_prevalence_by_location_age_group_by_5);
    EXPECT_AGE_ROWS(blood_slide_prevalence_by_location_age);
    EXPECT_AGE_ROWS(blood_slide_number_by_location_age);
    EXPECT_LOCATION_ROWS(fraction_of_positive_that_are_clinical_by_location);
    EXPECT_LOCATION_ROWS(total_number_of_bites_by_location);
    EXPECT_LOCATION_ROWS(total_number_of_bites_by_location_year);
    EXPECT_LOCATION_ROWS(person_days_by_location_year);
    EXPECT_AGE_ROWS(eir_by_location_year);
    EXPECT_LOCATION_ROWS(eir_by_location);
    EXPECT_LOCATION_ROWS(cumulative_clinical_episodes_by_location);
    EXPECT_AGE_ROWS(cumulative_clinical_episodes_by_location_age);
    EXPECT_AGE_ROWS(cumulative_clinical_episodes_by_location_age_group);
    EXPECT_AGE_ROWS(average_number_biten_by_location_person);
    EXPECT_LOCATION_ROWS(percentage_bites_on_top_20_by_location);
    EXPECT_LOCATION_ROWS(cumulative_discounted_ntf_by_location);
    EXPECT_LOCATION_ROWS(cumulative_ntf_by_location);
    EXPECT_LOCATION_ROWS(cumulative_tf_by_location);
    EXPECT_LOCATION_ROWS(cumulative_number_treatments_by_location);
    EXPECT_LOCATION_ROWS(today_tf_by_location);
    EXPECT_LOCATION_ROWS(today_number_of_treatments_by_location);
    EXPECT_LOCATION_ROWS(today_ritf_by_location);
    EXPECT_AGE_ROWS(total_number_of_treatments_60_by_location);
    EXPECT_AGE_ROWS(total_ritf_60_by_location);
    EXPECT_AGE_ROWS(total_tf_60_by_location);
    EXPECT_LOCATION_ROWS(current_ritf_by_location);
    EXPECT_LOCATION_ROWS(current_tf_by_location);
    EXPECT_LOCATION_ROWS(cumulative_mutants_by_location);
    EXPECT_THERAPY_ROWS(number_of_treatments_with_therapy_id);
    EXPECT_THERAPY_ROWS(number_of_treatments_success_with_therapy_id);
    EXPECT_THERAPY_ROWS(number_of_treatments_fail_with_therapy_id);
    EXPECT_AGE_ROWS(multiple_of_infection_by_location);
    EXPECT_LOCATION_ROWS(current_eir_by_location);
    EXPECT_LOCATION_ROWS(last_update_total_number_of_bites_by_location);
    EXPECT_AGE_ROWS(last_10_blood_slide_prevalence_by_location);
    EXPECT_AGE_ROWS(last_10_blood_slide_prevalence_by_location_age_class);
    EXPECT_AGE_ROWS(last_10_fraction_positive_that_are_clinical_by_location);
    EXPECT_AGE_ROWS(last_10_fraction_positive_that_are_clinical_by_location_age_class);
    EXPECT_AGE_ROWS(last_10_fraction_positive_that_are_clinical_by_location_age_class_by_5);
    EXPECT_LOCATION_ROWS(total_parasite_population_by_location);
    EXPECT_LOCATION_ROWS(number_of_positive_by_location);
    EXPECT_AGE_ROWS(total_parasite_population_by_location_age_group);
    EXPECT_AGE_ROWS(number_of_positive_by_location_age_group);
    EXPECT_AGE_ROWS(number_of_clinical_by_location_age_group);
    EXPECT_AGE_ROWS(number_of_clinical_by_location_age_group_by_5);
    EXPECT_AGE_ROWS(number_of_death_by_location_age_group);
    EXPECT_AGE_ROWS(number_of_untreated_cases_by_location_age_year);
    EXPECT_AGE_ROWS(number_of_treatments_by_location_age_year);
    EXPECT_AGE_ROWS(number_of_deaths_by_location_age_year);
    EXPECT_AGE_ROWS(number_of_malaria_deaths_treated_by_location_age_year);
    EXPECT_AGE_ROWS(number_of_malaria_deaths_non_treated_by_location_age_year);
    EXPECT_LOCATION_ROWS(monthly_number_of_treatment_by_location);
    EXPECT_LOCATION_ROWS(monthly_number_of_tf_by_location);
    EXPECT_LOCATION_ROWS(monthly_number_of_new_infections_by_location);
    EXPECT_LOCATION_ROWS(monthly_number_of_recrudescence_treatment_by_location);
    EXPECT_AGE_ROWS(monthly_number_of_recrudescence_treatment_by_location_age_class);
    EXPECT_AGE_ROWS(monthly_number_of_recrudescence_treatment_by_location_age);
    EXPECT_LOCATION_ROWS(monthly_number_of_clinical_episode_by_location);
    EXPECT_AGE_ROWS(monthly_number_of_clinical_episode_by_location_age);
    EXPECT_LOCATION_ROWS(monthly_number_of_mutation_events_by_location);
    EXPECT_AGE_ROWS(popsize_by_location_age);
    EXPECT_THERAPY_ROWS(today_tf_by_therapy);
    EXPECT_THERAPY_ROWS(today_number_of_treatments_by_therapy);
    EXPECT_THERAPY_ROWS(current_tf_by_therapy);
    EXPECT_THERAPY_ROWS(total_number_of_treatments_60_by_therapy);
    EXPECT_THERAPY_ROWS(total_tf_60_by_therapy);
    EXPECT_LOCATION_ROWS(monthly_treatment_failure_by_location);
    EXPECT_LOCATION_ROWS(monthly_nontreatment_by_location);
    EXPECT_AGE_ROWS(monthly_number_of_treatment_by_location_age_class);
    EXPECT_AGE_ROWS(monthly_number_of_clinical_episode_by_location_age_class);
    EXPECT_LOCATION_ROWS(births_by_location);
    EXPECT_LOCATION_ROWS(deaths_by_location);
    EXPECT_LOCATION_ROWS(malaria_deaths_by_location);
    EXPECT_LOCATION_ROWS(monthly_treatment_success_by_location);
    EXPECT_AGE_ROWS(monthly_nontreatment_by_location_age_class);
    EXPECT_AGE_ROWS(malaria_deaths_by_location_age_class);
    EXPECT_AGE_ROWS(monthly_number_of_treatment_by_location_therapy);
    EXPECT_AGE_ROWS(monthly_treatment_complete_by_location_therapy);
    EXPECT_AGE_ROWS(monthly_treatment_failure_by_location_age_class);
    EXPECT_AGE_ROWS(monthly_treatment_failure_by_location_therapy);
    EXPECT_AGE_ROWS(monthly_treatment_success_by_location_age_class);
    EXPECT_AGE_ROWS(monthly_treatment_success_by_location_therapy);
    EXPECT_LOCATION_ROWS(monthly_number_of_people_seeking_treatment_by_location_age_index);
    EXPECT_LOCATION_ROWS(mosquito_recombination_events_count);

    mdc_->set_tf_at_15(0.25);
    mdc_->set_mean_moi(1.5);
    mdc_->set_current_number_of_mutation_events_in_this_year(4);
    EXPECT_DOUBLE_EQ(mdc_->tf_at_15(), 0.25);
    EXPECT_DOUBLE_EQ(mdc_->mean_moi(), 1.5);
    EXPECT_EQ(mdc_->current_number_of_mutation_events_in_this_year(), 4);

#undef EXPECT_LOCATION_ROWS
#undef EXPECT_AGE_ROWS
#undef EXPECT_THERAPY_ROWS
}

TEST_F(ModelDataCollectorTest, PublicStatisticsAccessorsRoundTripTheirValues) {
#define ROUNDTRIP(accessor) \
    do { \
        const auto snapshot = mdc_->accessor(); \
        mdc_->set_##accessor(snapshot); \
        EXPECT_EQ(mdc_->accessor(), snapshot); \
    } while (false)

    ROUNDTRIP(total_immune_by_location);
    ROUNDTRIP(total_immune_by_location_age_class);
    ROUNDTRIP(total_immune_by_location_age);
    ROUNDTRIP(popsize_by_location);
    ROUNDTRIP(popsize_residence_by_location);
    ROUNDTRIP(popsize_by_location_age_class);
    ROUNDTRIP(popsize_by_location_age_class_by_5);
    ROUNDTRIP(popsize_by_location_hoststate);
    ROUNDTRIP(popsize_by_location_hoststate_age_class);
    ROUNDTRIP(blood_slide_prevalence_by_location);
    ROUNDTRIP(blood_slide_number_by_location_age_group);
    ROUNDTRIP(blood_slide_prevalence_by_location_age_group);
    ROUNDTRIP(blood_slide_number_by_location_age_group_by_5);
    ROUNDTRIP(blood_slide_prevalence_by_location_age_group_by_5);
    ROUNDTRIP(blood_slide_prevalence_by_location_age);
    ROUNDTRIP(blood_slide_number_by_location_age);
    ROUNDTRIP(fraction_of_positive_that_are_clinical_by_location);
    ROUNDTRIP(total_number_of_bites_by_location);
    ROUNDTRIP(total_number_of_bites_by_location_year);
    ROUNDTRIP(person_days_by_location_year);
    ROUNDTRIP(eir_by_location_year);
    ROUNDTRIP(eir_by_location);
    ROUNDTRIP(cumulative_clinical_episodes_by_location);
    ROUNDTRIP(cumulative_clinical_episodes_by_location_age);
    ROUNDTRIP(cumulative_clinical_episodes_by_location_age_group);
    ROUNDTRIP(average_number_biten_by_location_person);
    ROUNDTRIP(percentage_bites_on_top_20_by_location);
    ROUNDTRIP(cumulative_discounted_ntf_by_location);
    ROUNDTRIP(cumulative_ntf_by_location);
    ROUNDTRIP(cumulative_tf_by_location);
    ROUNDTRIP(cumulative_number_treatments_by_location);
    ROUNDTRIP(today_tf_by_location);
    ROUNDTRIP(today_number_of_treatments_by_location);
    ROUNDTRIP(today_ritf_by_location);
    ROUNDTRIP(total_number_of_treatments_60_by_location);
    ROUNDTRIP(total_ritf_60_by_location);
    ROUNDTRIP(total_tf_60_by_location);
    ROUNDTRIP(current_ritf_by_location);
    ROUNDTRIP(current_tf_by_location);
    ROUNDTRIP(cumulative_mutants_by_location);
    ROUNDTRIP(number_of_treatments_with_therapy_id);
    ROUNDTRIP(number_of_treatments_success_with_therapy_id);
    ROUNDTRIP(number_of_treatments_fail_with_therapy_id);
    ROUNDTRIP(multiple_of_infection_by_location);
    ROUNDTRIP(current_eir_by_location);
    ROUNDTRIP(last_update_total_number_of_bites_by_location);
    ROUNDTRIP(last_10_blood_slide_prevalence_by_location);
    ROUNDTRIP(last_10_blood_slide_prevalence_by_location_age_class);
    ROUNDTRIP(last_10_fraction_positive_that_are_clinical_by_location);
    ROUNDTRIP(last_10_fraction_positive_that_are_clinical_by_location_age_class);
    ROUNDTRIP(last_10_fraction_positive_that_are_clinical_by_location_age_class_by_5);
    ROUNDTRIP(total_parasite_population_by_location);
    ROUNDTRIP(number_of_positive_by_location);
    ROUNDTRIP(total_parasite_population_by_location_age_group);
    ROUNDTRIP(number_of_positive_by_location_age_group);
    ROUNDTRIP(number_of_clinical_by_location_age_group);
    ROUNDTRIP(number_of_clinical_by_location_age_group_by_5);
    ROUNDTRIP(number_of_death_by_location_age_group);
    ROUNDTRIP(number_of_untreated_cases_by_location_age_year);
    ROUNDTRIP(number_of_treatments_by_location_age_year);
    ROUNDTRIP(number_of_deaths_by_location_age_year);
    ROUNDTRIP(number_of_malaria_deaths_treated_by_location_age_year);
    ROUNDTRIP(number_of_malaria_deaths_non_treated_by_location_age_year);
    ROUNDTRIP(monthly_number_of_treatment_by_location);
    ROUNDTRIP(monthly_number_of_tf_by_location);
    ROUNDTRIP(monthly_number_of_new_infections_by_location);
    ROUNDTRIP(monthly_number_of_recrudescence_treatment_by_location);
    ROUNDTRIP(monthly_number_of_recrudescence_treatment_by_location_age_class);
    ROUNDTRIP(monthly_number_of_recrudescence_treatment_by_location_age);
    ROUNDTRIP(monthly_number_of_clinical_episode_by_location);
    ROUNDTRIP(monthly_number_of_clinical_episode_by_location_age);
    ROUNDTRIP(monthly_number_of_mutation_events_by_location);
    ROUNDTRIP(popsize_by_location_age);
    ROUNDTRIP(today_tf_by_therapy);
    ROUNDTRIP(today_number_of_treatments_by_therapy);
    ROUNDTRIP(current_tf_by_therapy);
    ROUNDTRIP(total_number_of_treatments_60_by_therapy);
    ROUNDTRIP(total_tf_60_by_therapy);
    ROUNDTRIP(monthly_treatment_failure_by_location);
    ROUNDTRIP(monthly_nontreatment_by_location);
    ROUNDTRIP(monthly_number_of_treatment_by_location_age_class);
    ROUNDTRIP(monthly_number_of_clinical_episode_by_location_age_class);
    ROUNDTRIP(births_by_location);
    ROUNDTRIP(deaths_by_location);
    ROUNDTRIP(malaria_deaths_by_location);
    ROUNDTRIP(monthly_treatment_success_by_location);
    ROUNDTRIP(monthly_nontreatment_by_location_age_class);
    ROUNDTRIP(malaria_deaths_by_location_age_class);
    ROUNDTRIP(monthly_number_of_treatment_by_location_therapy);
    ROUNDTRIP(monthly_treatment_complete_by_location_therapy);
    ROUNDTRIP(monthly_treatment_failure_by_location_age_class);
    ROUNDTRIP(monthly_treatment_failure_by_location_therapy);
    ROUNDTRIP(monthly_treatment_success_by_location_age_class);
    ROUNDTRIP(monthly_treatment_success_by_location_therapy);
    ROUNDTRIP(monthly_number_of_people_seeking_treatment_by_location_age_index);
    ROUNDTRIP(mosquito_recombination_events_count);

    mdc_->set_recording(true);
    EXPECT_TRUE(mdc_->get_recording());
    mdc_->set_recording(false);
    EXPECT_FALSE(mdc_->get_recording());

#undef ROUNDTRIP
}
