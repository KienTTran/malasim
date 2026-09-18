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

// New test: Age-based probability of seeking treatment counters
TEST_F(ModelDataCollectorTest, AgeBasedSeekingInitializeRecordZero) {
    // Release current model and create a modified test_input.yml that includes the age-based config
    Model::get_instance()->release();

    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &cfg){
        if (!cfg["epidemiological_parameters"]) cfg["epidemiological_parameters"] = YAML::Node(YAML::NodeType::Map);
        cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"] = YAML::Node(YAML::NodeType::Map);
        auto n = cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"];
        n["enable"] = true;
        n["type"] = "power";
        n["power"]["base"] = 0.9;
        n["power"]["exponent_source"] = "index";
        n["ages"] = std::vector<int>{0,5,10,15};
    });

    // Re-initialize model with modified config
    ASSERT_TRUE(Model::get_instance()->initialize());
    mdc_ = Model::get_mdc();
    mdc_->initialize();

    const int locations = Model::get_config()->number_of_locations();
    const int ages_count = static_cast<int>(
    Model::get_config()
        ->get_epidemiological_parameters()
        .get_age_based_probability_of_seeking_treatment()
        .get_ages().size());

    // Check dimensions
    ASSERT_EQ(static_cast<int>(mdc_->monthly_number_of_people_seeking_treatment_by_location_age_index().size()), locations);
    ASSERT_EQ(static_cast<int>(mdc_->monthly_number_of_people_seeking_treatment_by_location_age_index()[0].size()), ages_count);

    // Record some events
    mdc_->set_recording(true);
    mdc_->record_1_person_seeking_treatment_by_location_age_index(0, 0);
    mdc_->record_1_person_seeking_treatment_by_location_age_index(0, 2);
    mdc_->record_1_person_seeking_treatment_by_location_age_index(0, 2);
    mdc_->set_recording(false);

    EXPECT_EQ(mdc_->monthly_number_of_people_seeking_treatment_by_location_age_index()[0][0], 1);
    EXPECT_EQ(mdc_->monthly_number_of_people_seeking_treatment_by_location_age_index()[0][1], 0);
    EXPECT_EQ(mdc_->monthly_number_of_people_seeking_treatment_by_location_age_index()[0][2], 2);

    // Monthly update should zero monthly counters
    mdc_->monthly_update();
    EXPECT_EQ(mdc_->monthly_number_of_people_seeking_treatment_by_location_age_index()[0][0], 0);
    EXPECT_EQ(mdc_->monthly_number_of_people_seeking_treatment_by_location_age_index()[0][2], 0);
}

// Test for AgeBasedProbabilityOfSeekingTreatment evaluation helper
TEST_F(ModelDataCollectorTest, AgeBasedSeekingEvaluationEnabledDisabled) {
    // Release and prepare a config with specific ages
    Model::get_instance()->release();

    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &cfg){
        if (!cfg["epidemiological_parameters"]) cfg["epidemiological_parameters"] = YAML::Node(YAML::NodeType::Map);
        cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"] = YAML::Node(YAML::NodeType::Map);
        auto n = cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"];
        n["enable"] = true;
        n["type"] = "power";
        n["power"]["base"] = 0.9;
        n["power"]["exponent_source"] = "index";
        n["ages"] = std::vector<int>{0,5,10,15,20,30,40};
    });

    ASSERT_TRUE(Model::get_instance()->initialize());
    const auto &ep = Model::get_config()->get_epidemiological_parameters();
    const auto &agecfg = ep.get_age_based_probability_of_seeking_treatment();

    // Enabled: age 2 -> idx 0 -> modifier 0.9^(0) = 1.0
    EXPECT_DOUBLE_EQ(agecfg.evaluate_for_age(2), 1.0);
    // Enabled: age 12 -> idx 2 (ages[2] == 10) -> modifier 0.9^(2) = 0.81
    EXPECT_NEAR(agecfg.evaluate_for_age(12), 0.81, 1e-12);

    // Now set disabled and re-check (should return 1.0 regardless)
    Model::get_instance()->release();
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &cfg){
        if (!cfg["epidemiological_parameters"]) cfg["epidemiological_parameters"] = YAML::Node(YAML::NodeType::Map);
        cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"] = YAML::Node(YAML::NodeType::Map);
        auto n = cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"];
        n["enable"] = false;
        n["type"] = "power";
        n["power"]["base"] = 0.9;
        n["power"]["exponent_source"] = "index";
        n["ages"] = std::vector<int>{0,5,10,15,20,30,40};
    });
    ASSERT_TRUE(Model::get_instance()->initialize());
    const auto &ep2 = Model::get_config()->get_epidemiological_parameters();
    const auto &agecfg2 = ep2.get_age_based_probability_of_seeking_treatment();
    EXPECT_DOUBLE_EQ(agecfg2.evaluate_for_age(2), 1.0);
    EXPECT_DOUBLE_EQ(agecfg2.evaluate_for_age(12), 1.0);
}

