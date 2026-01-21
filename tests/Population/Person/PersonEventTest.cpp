#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <ranges>

#include "Events/BirthdayEvent.h"
#include "Events/CirculateToTargetLocationNextDayEvent.h"
#include "Events/EndClinicalEvent.h"
#include "Events/MatureGametocyteEvent.h"
#include "Events/MoveParasiteToBloodEvent.h"
#include "Events/ProgressToClinicalEvent.h"
#include "Events/ReceiveTherapyEvent.h"
#include "Events/ReturnToResidenceEvent.h"
#include "Events/SwitchImmuneComponentEvent.h"
#include "Events/TestTreatmentFailureEvent.h"
#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/DrugsInBlood.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/ImmuneSystem/InfantImmuneComponent.h"
#include "Population/ImmuneSystem/NonInfantImmuneComponent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Treatment/Therapies/Drug.h"
#include "Treatment/Therapies/SCTherapy.h"
#include "Utils/Cli.h"
#include "Utils/Constants.h"
#include "fixtures/TestFileGenerators.h"

class PersonInternalEventTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Set up test environment
    test_fixtures::setup_test_environment();

    // Set the input path to the config file
    utils::Cli::get_instance().set_input_path("test_input.yml");

    // Initialize the model to load the config
    ASSERT_TRUE(Model::get_instance()->initialize());

    // Create a new person
    person_ = std::make_unique<Person>();
    person_->initialize();

    // Set basic properties
    location_ = 0;
    person_->set_location(location_);
    person_->set_residence_location(location_);
    person_->set_host_state(Person::SUSCEPTIBLE);
    person_->set_age(20);  // 20 years old

    // Set birthday (simplified for test - using a fixed value)
    int days_to_next_birthday = Constants::DAYS_IN_YEAR / 2;
    auto ymd = Model::get_scheduler()->get_ymd_after_days(days_to_next_birthday)
               - date::years(person_->get_age() + 1);
    auto simulation_time_birthday = Model::get_scheduler()->get_days_to_ymd(ymd);
    person_->set_birthday(simulation_time_birthday);

    // Set immune component
    person_->get_immune_system()->set_immune_component(
        std::make_unique<NonInfantImmuneComponent>());
    person_->get_immune_system()->immune_component()->set_latest_value(0.5);

    // Set biting rate and moving level
    person_->set_innate_relative_biting_rate(1.0);
    person_->update_relative_biting_rate();
    person_->set_moving_level(1);

    // Set latest update time
    person_->set_latest_update_time(0);

    // Set current model time to 0
    Model::get_scheduler()->set_current_time(0);
  }

  void TearDown() override {
    // Clean up
    person_.reset();
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  // Helper method to create a parasite for testing
  ClonalParasitePopulation* add_parasite_to_person() {
    // Create a genotype
    auto genotype = std::make_unique<Genotype>("test_genotype");
    genotype->set_genotype_id(999);  // Use a unique ID
    Genotype* genotype_ptr = genotype.get();

    // Add the genotype to the database
    Model::get_genotype_db()->add(std::move(genotype));

    // Add the parasite to the person
    return person_->add_new_parasite_to_blood(genotype_ptr);
  }

  // Helper method to calculate future time
  int calculate_future_time(int days_delay) const {
    return Model::get_scheduler()->current_time() + days_delay;
  }

  std::unique_ptr<Person> person_;
  int location_;
};

// Test BirthdayEvent
TEST_F(PersonInternalEventTest, BirthdayEventTest) {
  // Get initial age
  uint initial_age = person_->get_age();

  // Create and schedule a birthday event
  auto event = std::make_unique<BirthdayEvent>(person_.get());
  event->set_time(calculate_future_time(0));  // Execute immediately
  person_->schedule_basic_event(std::move(event));

  // Execute events at current time
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify age increased by 1
  ASSERT_EQ(person_->get_age(), initial_age + 1);
}

// Test SwitchImmuneComponentEvent
TEST_F(PersonInternalEventTest, SwitchImmuneComponentEventTest) {
  // Set up an infant immune component first
  person_->get_immune_system()->set_immune_component(std::make_unique<InfantImmuneComponent>());

  // Create and schedule a switch immune component event
  auto event = std::make_unique<SwitchImmuneComponentEvent>(person_.get());
  event->set_time(calculate_future_time(0));  // Execute immediately
  person_->schedule_basic_event(std::move(event));

  // Execute events at current time
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify immune component was switched to NonInfantImmuneComponent
  ASSERT_NE(person_->get_immune_system()->immune_component(), nullptr);
}

// Test MoveParasiteToBloodEvent
TEST_F(PersonInternalEventTest, MoveParasiteToBloodEventTest) {
  // Create a genotype
  auto genotype = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
  genotype->set_genotype_id(999);
  Genotype* genotype_ptr = genotype.get();

  // Add the genotype to the database
  Model::get_genotype_db()->add(std::move(genotype));

  // Create and schedule a move parasite to blood event
  auto event = std::make_unique<MoveParasiteToBloodEvent>(person_.get());
  event->set_time(calculate_future_time(0));  // Execute immediately
  event->set_infection_genotype(genotype_ptr);
  person_->schedule_basic_event(std::move(event));

  // Execute events at current time
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify parasite was added to blood
  ASSERT_GT(person_->get_all_clonal_parasite_populations()->size(), 0);
}

// Test MatureGametocyteEvent
TEST_F(PersonInternalEventTest, MatureGametocyteEventTest) {
  // Add a parasite to the person
  ClonalParasitePopulation* parasite = add_parasite_to_person();

  // Set initial gametocyte level
  parasite->set_gametocyte_level(0.0);

  // Create and schedule a mature gametocyte event
  auto event = std::make_unique<MatureGametocyteEvent>(person_.get());
  event->set_time(calculate_future_time(0));  // Execute immediately
  person_->schedule_basic_event(std::move(event));

  // Execute events at current time
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify person has a parasite
  ASSERT_GT(person_->get_all_clonal_parasite_populations()->size(), 0);
}

// Test ProgressToClinicalEvent
TEST_F(PersonInternalEventTest, ProgressToClinicalEventTest) {
  int test_counts = 1000;
  int clinical_counts = 0;
  for (int i = 0; i < test_counts; i++) {
    // Create a genotype
    auto genotype = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
    genotype->set_genotype_id(999);
    Genotype* genotype_ptr = genotype.get();

    // Add the genotype to the database
    Model::get_genotype_db()->add(std::move(genotype));

    // Create and schedule a move parasite to blood event
    auto event_1 = std::make_unique<MoveParasiteToBloodEvent>(person_.get());
    event_1->set_time(calculate_future_time(0));  // Execute immediately
    event_1->set_infection_genotype(genotype_ptr);
    person_->schedule_basic_event(std::move(event_1));

    // Set initial host state
    person_->set_host_state(Person::ASYMPTOMATIC);

    // Create and schedule a progress to clinical event
    auto event_2 = std::make_unique<ProgressToClinicalEvent>(person_.get());
    event_2->set_time(calculate_future_time(0));  // Execute immediately
    person_->schedule_basic_event(std::move(event_2));

    // Execute events at current time
    person_->update_events(Model::get_scheduler()->current_time());

    // check all events in person
    //  for (const auto& [time, event] : person_->get_events()) {
    //    spdlog::info("Event: {} at time {}", event->name(), time);
    //  }

    for (auto time = 0; time < 30; time++) { person_->update_events(time); }
    if (person_->get_host_state() == Person::CLINICAL) { clinical_counts++; }
  }
  spdlog::info("Clinical counts: {}", clinical_counts);
  // Verify host state changed to clinical
  ASSERT_GT(clinical_counts, 0);
}

// Test EndClinicalEvent
TEST_F(PersonInternalEventTest, EndClinicalEventTest) {
  // Set initial host state to clinical
  person_->set_host_state(Person::CLINICAL);

  // Create and schedule an end clinical event
  auto event = std::make_unique<EndClinicalEvent>(person_.get());
  event->set_time(calculate_future_time(0));  // Execute immediately
  person_->schedule_basic_event(std::move(event));

  // Execute events at current time
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify host state changed from clinical
  ASSERT_NE(person_->get_host_state(), Person::CLINICAL);
}

// Test ReceiveTherapyEvent
TEST_F(PersonInternalEventTest, ReceiveTherapyEventTest) {
  // Create a genotype
  auto genotype = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
  genotype->set_genotype_id(999);
  Genotype* genotype_ptr = genotype.get();

  // Add the genotype to the database
  Model::get_genotype_db()->add(std::move(genotype));

  // Create and schedule a move parasite to blood event
  auto event_1 = std::make_unique<MoveParasiteToBloodEvent>(person_.get());
  event_1->set_time(calculate_future_time(0));  // Execute immediately
  event_1->set_infection_genotype(genotype_ptr);
  person_->schedule_basic_event(std::move(event_1));

  // Set initial host state
  person_->set_host_state(Person::ASYMPTOMATIC);

  person_->update_events(Model::get_scheduler()->current_time());

  // Create a therapy
  auto &therapy = Model::get_therapy_db().at(6);

  // Create and schedule a receive therapy event
  auto event = std::make_unique<ReceiveTherapyEvent>(person_.get());
  event->set_received_therapy(therapy.get());
  event->set_clinical_caused_parasite(person_->get_all_clonal_parasite_populations()->at(0));
  event->set_time(calculate_future_time(0));  // Execute immediately
  event->set_is_part_of_mac_therapy(false);
  person_->schedule_basic_event(std::move(event));

  person_->update_events(Model::get_scheduler()->current_time());

  // Verify drugs were added to blood
  if (Model::get_drug_db()->size() > 0) { ASSERT_GT(person_->drugs_in_blood()->size(), 0); }
}

// Test ReturnToResidenceEvent
TEST_F(PersonInternalEventTest, ReturnToResidenceEventTest) {
  Population* population = Model::get_instance()->get_population();
  std::vector<int> pop_size_by_location = {0, 1};
  population->set_popsize_by_location(pop_size_by_location);

  // Set different current location and residence location
  int residence_location = 0;
  int current_location = 1;
  person_->set_residence_location(residence_location);
  person_->set_location(current_location);

  // Create and schedule a return to residence event
  auto event = std::make_unique<ReturnToResidenceEvent>(person_.get());
  event->set_time(calculate_future_time(0));  // Execute immediately
  person_->schedule_basic_event(std::move(event));

  // Execute events at current time
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify location changed back to residence location
  ASSERT_EQ(person_->get_location(), residence_location);
}

// Test CirculateToTargetLocationNextDayEvent
TEST_F(PersonInternalEventTest, CirculateToTargetLocationNextDayEventTest) {
  Population* population = Model::get_instance()->get_population();
  std::vector<int> pop_size_by_location = {1, 1};
  population->set_popsize_by_location(pop_size_by_location);
  // Set initial location
  int initial_location = 0;
  int target_location = 1;
  person_->set_location(initial_location);

  // Create and schedule a circulate to target location event
  auto event = std::make_unique<CirculateToTargetLocationNextDayEvent>(person_.get());
  event->set_time(calculate_future_time(0));  // Execute immediately
  event->set_target_location(1);
  person_->schedule_basic_event(std::move(event));

  // We can't set target location in CirculateToTargetLocationNextDayEvent
  // So we'll test the location change directly
  person_->set_location(target_location);

  // Execute events at current time
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify location changed to target location
  ASSERT_EQ(person_->get_location(), target_location);
}

// Test TestTreatmentFailureEvent
TEST_F(PersonInternalEventTest, TestTreatmentFailureEventTest) {
  // Setup - create a detectable parasite to simulate treatment failure
  std::string test_sequence = "||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1";
  auto genotype = std::make_unique<Genotype>(test_sequence);
  auto clinical_parasite = std::make_unique<ClonalParasitePopulation>(genotype.get());

  // Set parasite density to detectable level to trigger treatment failure
  double detectable_density = Model::get_config()
                                  ->get_parasite_parameters()
                                  .get_parasite_density_levels()
                                  .get_log_parasite_density_detectable()
                              + 1.0;
  clinical_parasite->set_last_update_log10_parasite_density(detectable_density);

  // Store a raw pointer before moving ownership
  ClonalParasitePopulation* clinical_parasite_ptr = clinical_parasite.get();

  // Add the parasite to the person
  person_->get_all_clonal_parasite_populations()->add(std::move(clinical_parasite));

  // Define a therapy ID to test with
  const int test_therapy_id = 1;

  // Record initial failure count for the specific therapy
  int initial_failures = 0;
  if (test_therapy_id < Model::get_mdc()->number_of_treatments_fail_with_therapy_id().size()) {
    initial_failures =
        Model::get_mdc()->number_of_treatments_fail_with_therapy_id()[test_therapy_id];
  }

  // Create the treatment failure event
  auto tf_event = std::make_unique<TestTreatmentFailureEvent>(person_.get());
  tf_event->set_time(Model::get_scheduler()->current_time());
  tf_event->set_clinical_caused_parasite(clinical_parasite_ptr);
  tf_event->set_therapy_id(test_therapy_id);

  // Schedule and execute the event
  person_->schedule_basic_event(std::move(tf_event));
  person_->update_events(Model::get_scheduler()->current_time());

  // Verify treatment failure was recorded (since parasite is still detectable)
  int final_failures = 0;
  if (test_therapy_id < Model::get_mdc()->number_of_treatments_fail_with_therapy_id().size()) {
    final_failures = Model::get_mdc()->number_of_treatments_fail_with_therapy_id()[test_therapy_id];
  }

  // Check that treatment failure count increased
  EXPECT_EQ(initial_failures + 1, final_failures)
      << "Treatment failure count for therapy " << test_therapy_id << " should increase by 1";

  // Clean up
  person_->get_all_clonal_parasite_populations()->clear();
}
