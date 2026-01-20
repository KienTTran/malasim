#include <gtest/gtest.h>
#include <memory>

#include "Simulation/Model.h"
#include "Population/Person/Person.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/ImmuneSystem/InfantImmuneComponent.h"
#include "Population/ImmuneSystem/NonInfantImmuneComponent.h"
#include "Parasites/Genotype.h"
#include "Population/DrugsInBlood.h"
#include "Treatment/Therapies/DrugType.h"
#include "Treatment/Therapies/SCTherapy.h"
#include "Treatment/Therapies/Drug.h"
#include "Utils/Cli.h"
#include "Utils/Constants.h"

class PersonGenerateIndividualTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set the input path to the config file
        utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
        
        // Initialize the model to load the config
        ASSERT_TRUE(Model::get_instance()->initialize());
        
        // Create a new person
        person_ = std::make_unique<Person>();
    }

    void TearDown() override {
        // Clean up
        person_.reset();
    }

    std::unique_ptr<Person> person_;
};

// This test mimics the logic of Population::generate_individual to ensure
// a Person is properly initialized according to the simulation framework
TEST_F(PersonGenerateIndividualTest, InitializePersonLikePopulationGenerateIndividual) {
    // 1. Initialize the person as in Population::generate_individual
    person_->initialize();
    
    // 2. Set location and residence location
    int location = 0;
    person_->set_location(location);
    person_->set_residence_location(location);
    
    // 3. Set host state to SUSCEPTIBLE
    person_->set_host_state(Person::SUSCEPTIBLE);
    
    // 4. Set age based on age class (using age class 1 for this test)
    int age_class = 1;
    uint age_from = (age_class == 0) ? 0 
                                    : Model::get_config()
                                          ->get_population_demographic()
                                          .get_initial_age_structure()[age_class - 1];
    uint age_to = Model::get_config()->get_population_demographic().get_initial_age_structure()[age_class];
    
    // Use a fixed age within the range for testing (instead of random)
    int age = static_cast<int>(age_from + (age_to - age_from) / 2);
    person_->set_age(age);
    
    // 5. Set birthday (simplified for test - using a fixed value)
    int days_to_next_birthday = Constants::DAYS_IN_YEAR / 2;
    auto ymd = Model::get_scheduler()->get_ymd_after_days(days_to_next_birthday) - date::years(person_->get_age() + 1);
    auto simulation_time_birthday = Model::get_scheduler()->get_days_to_ymd(ymd);
    person_->set_birthday(simulation_time_birthday);
    
    // 6. Schedule birthday event
    person_->schedule_birthday_event(days_to_next_birthday);
    
    // 7. Set immune component based on age
    if (simulation_time_birthday + Constants::DAYS_IN_YEAR / 2 >= 0) {
        person_->get_immune_system()->set_immune_component(std::make_unique<InfantImmuneComponent>());
        person_->schedule_switch_immune_component_event(simulation_time_birthday + (Constants::DAYS_IN_YEAR / 2));
    } else {
        person_->get_immune_system()->set_immune_component(std::make_unique<NonInfantImmuneComponent>());
    }
    
    // 8. Set immune value (using a fixed value for test)
    double immune_value = 0.7;
    person_->get_immune_system()->immune_component()->set_latest_value(immune_value);
    person_->get_immune_system()->set_increase(false);
    
    // 9. Set biting rate
    person_->set_innate_relative_biting_rate(
        Person::draw_random_relative_biting_rate(Model::get_random(), Model::get_config()));
    person_->update_relative_biting_rate();
    
    // 10. Set moving level
    auto& movement_settings = Model::get_config()->get_movement_settings();
    person_->set_moving_level(
        movement_settings.get_moving_level_generator().draw_random_level(Model::get_random()));
    
    // 11. Set latest update time
    person_->set_latest_update_time(0);
    
    // 12. Generate probability of being present at MDA
    person_->generate_prob_present_at_mda_by_age();
    
    // 13. Add a parasite to the person - use a new genotype instead of accessing from DB
    auto genotype = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
    genotype->set_genotype_id(0); // Use a unique ID that won't conflict
    Genotype* genotype_ptr = genotype.get();
    
    // Add the genotype to the database properly
    Model::get_genotype_db()->add(std::move(genotype));
    
    // Now add the parasite using the pointer
    auto* parasite = person_->add_new_parasite_to_blood(genotype_ptr);
    
    // 14. Use an existing therapy from the database if available
    // Check if therapy database has entries
    if (Model::get_therapy_db().size() > 0) {
        // Use the first therapy in the database
        auto& therapy = Model::get_therapy_db().at(0);
        if (therapy != nullptr) {
            person_->receive_therapy(therapy.get(), parasite, false);
        }
    } else {
        // Create a simple therapy if none exists in the database
        auto sc_therapy = std::make_unique<SCTherapy>();
        sc_therapy->set_id(999); // Use a unique ID
        
        // Add a drug if available in the drug database
        if (Model::get_drug_db()->size() > 0) {
            sc_therapy->add_drug(0);
            person_->receive_therapy(sc_therapy.get(), parasite, false);
        }
    }
    
    // Verify all properties are set correctly
    ASSERT_EQ(person_->get_location(), location);
    ASSERT_EQ(person_->get_residence_location(), location);
    ASSERT_EQ(person_->get_host_state(), Person::SUSCEPTIBLE);
    ASSERT_EQ(person_->get_age(), static_cast<uint>(age));
    ASSERT_NE(person_->get_immune_system(), nullptr);
    ASSERT_NE(person_->get_immune_system()->immune_component(), nullptr);
    ASSERT_DOUBLE_EQ(person_->get_immune_system()->immune_component()->get_current_value(), immune_value);
    ASSERT_GT(person_->get_innate_relative_biting_rate(), 0.0);
    ASSERT_GT(person_->get_current_relative_biting_rate(), 0.0);
    ASSERT_GE(person_->get_moving_level(), 0);
    ASSERT_EQ(person_->get_latest_update_time(), 0);
    
    // Verify parasite was added
    ASSERT_NE(parasite, nullptr);
    ASSERT_TRUE(person_->get_all_clonal_parasite_populations()->contain(parasite));
    
    // Verify drug was added (if therapy was available)
    // This is a soft assertion since we might not have drugs in the test environment
    if (Model::get_drug_db()->size() > 0 || Model::get_therapy_db().size() > 0) {
        EXPECT_GT(person_->drugs_in_blood()->size(), 0);
    }
}
