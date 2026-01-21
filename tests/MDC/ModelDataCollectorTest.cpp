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
#include "Utils/Cli.h"
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
        utils::Cli::get_instance().set_input_path("test_input.yml");
        
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
TEST_F(ModelDataCollectorTest, InitializeCorrectly) {
    // Re-initialize to test
    mdc_->initialize();
    
    // Verify the state after initialization
    ASSERT_EQ(mdc_->popsize_by_location().size(), Model::get_config()->number_of_locations());
    ASSERT_EQ(mdc_->blood_slide_prevalence_by_location().size(), Model::get_config()->number_of_locations());
    
    // Verify vectors are correctly initialized to zero
    for (size_t i = 0; i < mdc_->popsize_by_location().size(); i++) {
        EXPECT_EQ(mdc_->popsize_by_location()[i], 0);
        EXPECT_EQ(mdc_->blood_slide_prevalence_by_location()[i], 0.0);
    }
}

// Test begin_time_step functionality
TEST_F(ModelDataCollectorTest, BeginTimeStepInitializesCorrectly) {
    setupInitialState();
    
    // Set up some values that should be reset
    const int location = 0;
    mdc_->today_number_of_treatments_by_location()[location] = 5;
    mdc_->today_tf_by_location()[location] = 3;
    mdc_->today_ritf_by_location()[location] = 2;
    
    // Call begin_time_step
    mdc_->begin_time_step();
    
    // Verify values are reset
    EXPECT_EQ(mdc_->today_number_of_treatments_by_location()[location], 0);
    EXPECT_EQ(mdc_->today_tf_by_location()[location], 0);
    EXPECT_EQ(mdc_->today_ritf_by_location()[location], 0);
}

// Test record_1_treatment function
TEST_F(ModelDataCollectorTest, RecordTreatmentIncrementsCounts) {
    setupInitialState();
    
    // Record a treatment
    int location = 0;
    int age = 25;
    int age_class = 2;
    int therapy_id = 1;

    mdc_->set_recording(true);
    mdc_->record_1_treatment(location, age, age_class, therapy_id);
    mdc_->set_recording(false);
    
    // Verify counts incremented
    EXPECT_EQ(mdc_->today_number_of_treatments_by_location()[location], 1);
    EXPECT_EQ(mdc_->monthly_number_of_treatment_by_location()[location], 1);
    EXPECT_EQ(mdc_->monthly_number_of_treatment_by_location_age_class()[location][age_class], 1);
    EXPECT_EQ(mdc_->monthly_number_of_treatment_by_location_therapy()[location][therapy_id], 1);
}

// Test record_1_tf function (treatment failure)
TEST_F(ModelDataCollectorTest, RecordTreatmentFailureIncrementsCounts) {
    setupInitialState();
    
    // Record a treatment failure
    int location = 0;
    bool by_drug = true;

    mdc_->set_recording(true);
    mdc_->record_1_tf(location, by_drug);
    mdc_->set_recording(false);
    
    // Verify counts incremented
    EXPECT_EQ(mdc_->today_tf_by_location()[location], 1);
    EXPECT_EQ(mdc_->monthly_number_of_tf_by_location()[location], 1);
}

// Test record_1_clinical_episode function
TEST_F(ModelDataCollectorTest, RecordClinicalEpisodeIncrementsCounts) {
    setupInitialState();
    
    // Record a clinical episode
    int location = 0;
    int age = 25;
    int age_class = 2;
    
    mdc_->set_recording(true);
    mdc_->collect_1_clinical_episode(location, age, age_class);
    mdc_->set_recording(false);
    
    // Verify counts incremented
    EXPECT_EQ(mdc_->monthly_number_of_clinical_episode_by_location()[location], 1);
    if (age < 100) {
        EXPECT_EQ(mdc_->monthly_number_of_clinical_episode_by_location_age()[location][age], 1);
    }
    EXPECT_EQ(mdc_->monthly_number_of_clinical_episode_by_location_age_class()[location][age_class], 1);
    EXPECT_EQ(mdc_->cumulative_clinical_episodes_by_location()[location], 1);
}

// Test record_1_mutation function
TEST_F(ModelDataCollectorTest, RecordMutationIncrementsCounts) {
    setupInitialState();
    
    // Record a mutation using properly initialized genotypes
    int location = 0;
    
    mdc_->set_recording(true);
    mdc_->record_1_mutation(location, from_genotype_ptr_, to_genotype_ptr_);
    mdc_->set_recording(false);
    
    // Verify counts incremented
    EXPECT_EQ(mdc_->cumulative_mutants_by_location()[location], 1);
    EXPECT_EQ(mdc_->monthly_number_of_mutation_events_by_location()[location], 1);
    EXPECT_EQ(mdc_->current_number_of_mutation_events(), 1);
}

// Test update_person_days_by_years function
TEST_F(ModelDataCollectorTest, UpdatePersonDays) {
    setupInitialState();
    
    // Update person days
    int location = 0;
    int days = 30;
    
    mdc_->set_recording(true);
    mdc_->update_person_days_by_years(location, days);
    mdc_->set_recording(false);
    
    // Verify person days updated
    EXPECT_EQ(mdc_->person_days_by_location_year()[location], 30);
    
    // Update again
    mdc_->set_recording(true);
    mdc_->update_person_days_by_years(location, days);
    mdc_->set_recording(false);
    
    // Verify accumulation
    EXPECT_EQ(mdc_->person_days_by_location_year()[location], 60);
}

// Test collect_number_of_bites function
TEST_F(ModelDataCollectorTest, CollectNumberOfBites) {
    setupInitialState();
    
    // Collect bites
    int location = 0;
    int number_of_bites = 50;

    mdc_->set_recording(true);
    mdc_->collect_number_of_bites(location, number_of_bites);
    mdc_->set_recording(false);
    
    // Verify bites collected
    EXPECT_EQ(mdc_->total_number_of_bites_by_location()[location], 50);
    EXPECT_EQ(mdc_->total_number_of_bites_by_location_year()[location], 50);
    
    // Collect more bites
    mdc_->set_recording(true);
    mdc_->collect_number_of_bites(location, number_of_bites);
    mdc_->set_recording(false);
    
    // Verify accumulation
    EXPECT_EQ(mdc_->total_number_of_bites_by_location()[location], 100);
    EXPECT_EQ(mdc_->total_number_of_bites_by_location_year()[location], 100);
}

// Test monthly_update function
// TEST_F(ModelDataCollectorTest, MonthlyUpdateResetsCounters) {
//     setupInitialState();
//
//     // Set values that should be reset
//     mdc_->monthly_number_of_treatment_by_location()[0] = 10;
//     mdc_->monthly_number_of_clinical_episode_by_location()[0] = 5;
//     mdc_->monthly_treatment_failure_by_location()[0] = 3;
//
//     // Call monthly_update
//     mdc_->monthly_update();
//
//     // Verify values are reset
//     EXPECT_EQ(mdc_->monthly_number_of_treatment_by_location()[0], 0);
//     EXPECT_EQ(mdc_->monthly_number_of_clinical_episode_by_location()[0], 0);
//     EXPECT_EQ(mdc_->monthly_treatment_failure_by_location()[0], 0);
// }

// Test end_of_time_step function and tf calculation
TEST_F(ModelDataCollectorTest, EndOfTimeStepCalculatesTF) {
    setupInitialState();
    
    // Setup some initial data
    const int location = 0;
    const int window_size = Model::get_config()->get_epidemiological_parameters().get_tf_window_size();
    const int current_time = Model::get_scheduler()->current_time();
    const int time_index = current_time % window_size;
    
    // If we're in data collection period
    if (current_time >= Model::get_config()->get_simulation_timeframe().get_start_collect_data_day()) {
        // Set today's values
        mdc_->today_number_of_treatments_by_location()[location] = 10;
        mdc_->today_tf_by_location()[location] = 2;
        
        // Call end of time step to process
        mdc_->set_recording(true);
        mdc_->end_of_time_step();
        mdc_->set_recording(false);
        
        // Verify these values were stored in the window arrays
        EXPECT_EQ(mdc_->total_number_of_treatments_60_by_location()[location][time_index], 10);
        EXPECT_EQ(mdc_->total_tf_60_by_location()[location][time_index], 2);
        
        // We'd need to set multiple days of data to verify the TF rate calculation
        // But we can at least verify the calculation logic doesn't crash
        EXPECT_GE(mdc_->current_tf_by_location()[location], 0.0);
        EXPECT_LE(mdc_->current_tf_by_location()[location], 1.0);
    }
}

// Integration test for multiple operations
TEST_F(ModelDataCollectorTest, IntegratedStatisticsTest) {
    setupInitialState();
    
    const int location = 0;
    
    // Enable recording for all operations
    mdc_->set_recording(true);
    
    // Record multiple events to test integration
    
    // Record treatments
    mdc_->record_1_treatment(location, 25, 2, 1);
    mdc_->record_1_treatment(location, 35, 3, 1);
    mdc_->record_1_treatment(location, 45, 4, 2);
    
    // Record clinical episodes
    mdc_->collect_1_clinical_episode(location, 25, 2);
    mdc_->collect_1_clinical_episode(location, 35, 3);
    
    // Record treatment failures
    mdc_->record_1_tf(location, true);
    mdc_->record_1_tf(location, true);
    
    // Record mutations using properly initialized genotypes
    mdc_->record_1_mutation(location, from_genotype_ptr_, to_genotype_ptr_);
    
    // Collect bites
    mdc_->collect_number_of_bites(location, 100);
    
    // Update person days
    mdc_->update_person_days_by_years(location, 30);
    
    // Disable recording after all operations
    mdc_->set_recording(false);
    
    // Verify integrated statistics
    EXPECT_EQ(mdc_->today_number_of_treatments_by_location()[location], 3);
    EXPECT_EQ(mdc_->today_tf_by_location()[location], 2);
    EXPECT_EQ(mdc_->cumulative_clinical_episodes_by_location()[location], 2);
    EXPECT_EQ(mdc_->cumulative_mutants_by_location()[location], 1);
    EXPECT_EQ(mdc_->total_number_of_bites_by_location()[location], 100);
    EXPECT_EQ(mdc_->person_days_by_location_year()[location], 30);
}
