#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "Core/Scheduler/Scheduler.h"
#include "Events/MoveParasiteToBloodEvent.h"
#include "Events/ProgressToClinicalEvent.h"
#include "Events/ReceiveTherapyEvent.h"
#include "Events/TestTreatmentFailureEvent.h"
#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/DrugsInBlood.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Treatment/Therapies/Drug.h"
#include "Treatment/Therapies/DrugType.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Constants.h"
#include "fixtures/TestFileGenerators.h"

/**
 * Test class for testing the recrudescence functionality in the Person class.
 * This tests the determine_symptomatic_recrudescence method and related behavior.
 */
class PersonRecrudescenceTest : public ::testing::Test {
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

    // Create a new person
    person_ = std::make_unique<Person>();
    person_->initialize();

    // Initialize the clinical parasite - use proper constructors based on API
    // First create genotype with proper constructor
    std::string test_sequence = "||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1";  // Test sequence
    genotype_ = std::make_unique<Genotype>(test_sequence);

    // Create parasite population with just the genotype
    clinical_parasite_ = std::make_unique<ClonalParasitePopulation>(genotype_.get());
  }

  void TearDown() override {
    // Reset/release the model resources between tests
    Model::get_instance()->release();

    // Clear our own resources
    person_.reset();
    genotype_.reset();
    clinical_parasite_.reset();

    test_fixtures::cleanup_test_files();
  }

  // Helper method to set up a person with specific age and drugs
  void setupPerson(int age, bool hasDrugs) {
    // Initialize the person first
    person_->initialize();

    // Set basic properties
    person_->set_age(age);
    person_->set_host_state(Person::SUSCEPTIBLE);
    person_->set_location(0);  // Or pass a location parameter if needed
    person_->set_residence_location(0);

    // Set up birthday related properties
    auto days_to_next_birthday =
        static_cast<int>(Model::get_random()->random_uniform((Constants::DAYS_IN_YEAR))) + 1;
    auto ymd = Model::get_scheduler()->get_ymd_after_days(days_to_next_birthday)
               - date::years(person_->get_age() + 1);
    auto simulation_time_birthday = Model::get_scheduler()->get_days_to_ymd(ymd);
    person_->set_birthday(simulation_time_birthday);
    person_->schedule_birthday_event(days_to_next_birthday);

    // Set infant immune mode until six months
    if (simulation_time_birthday + Constants::DAYS_IN_YEAR / 2 >= 0) {
      if (person_->get_age() > 0) {
        spdlog::error("Error in calculating simulation_time_birthday");
      }
      person_->get_immune_system()->initialize_as_infant();
      // schedule for switch
      person_->schedule_switch_immune_system_mode_event(simulation_time_birthday
                                                        + (Constants::DAYS_IN_YEAR / 2));
    }

    // Set immune values
    auto immune_value = Model::get_random()->random_beta(
        Model::get_config()->get_immune_system_parameters().get_alpha_immune(),
        Model::get_config()->get_immune_system_parameters().get_beta_immune());
    person_->get_immune_system()->set_latest_immune_value(immune_value);
    person_->get_immune_system()->set_increase(false);

    // Set biting rate related properties
    person_->set_innate_relative_biting_rate(
        Person::draw_random_relative_biting_rate(Model::get_random(), Model::get_config()));
    person_->update_relative_biting_rate();

    // Set moving level
    auto &movement_settings = Model::get_config()->get_movement_settings();
    person_->set_moving_level(
        movement_settings.get_moving_level_generator().draw_random_level(Model::get_random()));

    person_->set_latest_update_time(0);
    person_->generate_prob_present_at_mda_by_age();

    // Handle drugs in blood
    if (hasDrugs) {
      // Create a therapy
      auto &therapy = Model::get_therapy_db().at(6);

      // Create and schedule a receive therapy event
      auto event = std::make_unique<ReceiveTherapyEvent>(person_.get());
      event->set_received_therapy(therapy.get());
      event->set_clinical_caused_parasite(clinical_parasite_.get());
      event->set_time(Model::get_scheduler()->current_time());  // Execute immediately
      event->set_is_part_of_mac_therapy(false);
      person_->schedule_basic_event(std::move(event));

      person_->update_events(Model::get_scheduler()->current_time());
    }
  }

  std::unique_ptr<Person> person_;
  std::unique_ptr<Genotype> genotype_;
  std::unique_ptr<ClonalParasitePopulation> clinical_parasite_;
};

// Test recrudescence with symptomatic outcome for a young child
TEST_F(PersonRecrudescenceTest, SymptomaticRecrudescenceYoungChild) {
  // Set up a young child
  setupPerson(5, false);  // 5 year old, no drugs

  // We'll use the actual random generator but with controlled conditions
  // Set the parasite density to asymptomatic level first
  clinical_parasite_->set_last_update_log10_parasite_density(
      Model::get_config()
          ->get_parasite_parameters()
          .get_parasite_density_levels()
          .get_log_parasite_density_asymptomatic());

  // Remove the conflicting MoveParasiteToBloodEvent scheduling.
  // The clinical_parasite_ will be evaluated independently.

  // Execute determine_symptomatic_recrudescence
  person_->determine_symptomatic_recrudescence(clinical_parasite_.get());

  // Verify the recurrence status (could be WITH_SYMPTOM or WITHOUT_SYMPTOM depending on random
  // values)
  if (person_->get_recurrence_status() == Person::RecurrenceStatus::WITH_SYMPTOM) {
    // Check that we have the correct update function
    EXPECT_EQ(Model::progress_to_clinical_update_function(), clinical_parasite_->update_function());

    // Verify that a clinical recurrence event was scheduled
    bool found_recurrence_event = false;
    for (const auto &[time, event] : person_->get_events()) {
      auto* progress_event = dynamic_cast<ProgressToClinicalEvent*>(event.get());
      if (progress_event != nullptr
          && progress_event->clinical_caused_parasite() == clinical_parasite_.get()) {
        EXPECT_TRUE(progress_event->is_recurrence());
        found_recurrence_event = true;

        // Verify the event is scheduled for a reasonable time (7-54 days in the future)
        int current_time = Model::get_scheduler()->current_time();
        int days_to_clinical = progress_event->get_time() - current_time;
        EXPECT_GE(days_to_clinical, 7);
        EXPECT_LE(days_to_clinical, 54);
        break;
      }
    }
    EXPECT_TRUE(found_recurrence_event) << "Clinical recurrence event was not scheduled";
  } else {
    // If asymptomatic, should have different update functions
    EXPECT_TRUE(clinical_parasite_->update_function() == Model::immunity_clearance_update_function()
                || clinical_parasite_->update_function() == Model::having_drug_update_function());
  }
}

// Test recrudescence with asymptomatic outcome for an adult with drugs
TEST_F(PersonRecrudescenceTest, RecrudescenceWithDrugs) {
  // Track outcomes
  int symptomatic_count = 0;
  int asymptomatic_count = 0;
  int other_count = 0;
  const int test_iterations = 100;

  for (int i = 0; i < test_iterations; i++) {
    // Setup
    setupPerson(30, true);  // 30 year old with drugs

    // Set parasite density above recrudescence threshold (>2, e.g., 100-1000 parasites/uL)
    clinical_parasite_->set_last_update_log10_parasite_density(3.0);  // 1000 parasites/uL

    // Execute
    person_->determine_symptomatic_recrudescence(clinical_parasite_.get());

    // Count outcomes
    if (person_->get_recurrence_status() == Person::RecurrenceStatus::WITH_SYMPTOM) {
      symptomatic_count++;
    } else if (person_->get_recurrence_status() == Person::RecurrenceStatus::WITHOUT_SYMPTOM) {
      asymptomatic_count++;
      // Check that with drugs and asymptomatic, we get the having_drug_update_function
      EXPECT_EQ(Model::having_drug_update_function(), clinical_parasite_->update_function());
    } else {
      other_count++;
    }

    // Reset between iterations
    person_->set_recurrence_status(Person::RecurrenceStatus::NONE);
    clinical_parasite_->set_update_function(nullptr);  // Reset the update function
  }

  // Log results
  std::cout << "RecrudescenceWithDrugs results after " << test_iterations
            << " iterations:" << std::endl;
  std::cout << "  Symptomatic: " << symptomatic_count << " ("
            << (static_cast<double>(symptomatic_count) / test_iterations) * 100.0 << "%)"
            << std::endl;
  std::cout << "  Asymptomatic: " << asymptomatic_count << " ("
            << (static_cast<double>(asymptomatic_count) / test_iterations) * 100.0 << "%)"
            << std::endl;
  std::cout << "  Other: " << other_count << " ("
            << (static_cast<double>(other_count) / test_iterations) * 100.0 << "%)" << std::endl;

  // Verify both outcomes occur with reasonable frequency
  // Since this is random, we can't be too strict, but we expect both outcomes to occur
  EXPECT_GT(asymptomatic_count, 0) << "Expected some asymptomatic cases";
  EXPECT_GT(symptomatic_count, 0) << "Expected some symptomatic cases";
}

// Test high parasite density adjustment for asymptomatic cases
TEST_F(PersonRecrudescenceTest, AsymptomaticDensityAdjustment) {
  // Track outcomes and density adjustments
  int symptomatic_count = 0;
  int asymptomatic_count = 0;
  int asymptomatic_density_adjusted_count = 0;
  const int test_iterations = 100;

  for (int i = 0; i < test_iterations; i++) {
    // Setup
    setupPerson(30, false);  // 30 year old, no drugs

    // Set high parasite density - DEFINITELY above asymptomatic threshold
    const double asymptomatic_threshold = Model::get_config()
                                              ->get_parasite_parameters()
                                              .get_parasite_density_levels()
                                              .get_log_parasite_density_asymptomatic();
    const double high_density = asymptomatic_threshold + 2.0;

    // Verify our test setup is correct
    ASSERT_GT(high_density, asymptomatic_threshold)
        << "Test setup error: High density should be above threshold";

    clinical_parasite_->set_last_update_log10_parasite_density(high_density);

    // Store original density for comparison
    const double original_density = clinical_parasite_->last_update_log10_parasite_density();

    // Execute
    person_->determine_symptomatic_recrudescence(clinical_parasite_.get());

    // Track outcomes
    if (person_->get_recurrence_status() == Person::RecurrenceStatus::WITHOUT_SYMPTOM) {
      asymptomatic_count++;

      // Density should be reduced to asymptomatic level or below for ALL asymptomatic cases
      // since we set it well above the threshold
      const double new_density = clinical_parasite_->last_update_log10_parasite_density();

      if (new_density < original_density) {
        asymptomatic_density_adjusted_count++;

        // The density is adjusted using random_normal_truncated with mean=asymptomatic_threshold
        // and std_dev=0.1, so values can be slightly above or below the threshold. We'll allow a
        // reasonable range.
        EXPECT_NEAR(new_density, asymptomatic_threshold, 0.3)
            << "Adjusted density should be close to asymptomatic threshold (within 0.3)";
      } else {
        // This should never happen in our test - log detailed info if it does
        std::cout << "ERROR: Density not adjusted for asymptomatic case in iteration " << i
                  << std::endl;
        std::cout << "  Original density: " << original_density << std::endl;
        std::cout << "  New density: " << new_density << std::endl;
        std::cout << "  Asymptomatic threshold: " << asymptomatic_threshold << std::endl;
      }
    } else if (person_->get_recurrence_status() == Person::RecurrenceStatus::WITH_SYMPTOM) {
      symptomatic_count++;
    }

    // Reset between iterations
    person_->set_recurrence_status(Person::RecurrenceStatus::NONE);
    clinical_parasite_->set_update_function(nullptr);  // Reset the update function
  }

  // Log results
  std::cout << "AsymptomaticDensityAdjustment results after " << test_iterations
            << " iterations:" << std::endl;
  std::cout << "  Symptomatic: " << symptomatic_count << " ("
            << (static_cast<double>(symptomatic_count) / test_iterations) * 100.0 << "%)"
            << std::endl;
  std::cout << "  Asymptomatic: " << asymptomatic_count << " ("
            << (static_cast<double>(asymptomatic_count) / test_iterations) * 100.0 << "%)"
            << std::endl;
  std::cout << "  Asymptomatic with density adjusted: " << asymptomatic_density_adjusted_count
            << " ("
            << (asymptomatic_count > 0 ? (static_cast<double>(asymptomatic_density_adjusted_count)
                                          / asymptomatic_count)
                                             * 100.0
                                       : 0)
            << "%)" << std::endl;

  // Verify we got at least some asymptomatic cases to test the density adjustment
  EXPECT_GT(asymptomatic_count, 0) << "Expected some asymptomatic cases to test density adjustment";

  // For all asymptomatic cases, verify the density was properly adjusted
  EXPECT_EQ(asymptomatic_count, asymptomatic_density_adjusted_count)
      << "All asymptomatic cases should have their parasite density adjusted";
}
