#include <gtest/gtest.h>

#include <memory>
#include <ranges>

#include "Events/ProgressToClinicalEvent.h"
#include "Events/TestTreatmentFailureEvent.h"
#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Constants.h"
#include "fixtures/TestFileGenerators.h"

class PersonRecrudescenceTreatmentFailureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
    person_ = std::make_unique<Person>();
    person_->initialize();
    genotype_ = std::make_unique<Genotype>("||||YF1||TTHFIMG,x||||||FNCMYRIPRPCRA|1");
    clinical_parasite_ = std::make_unique<ClonalParasitePopulation>(genotype_.get());
  }

  void TearDown() override {
    clinical_parasite_.reset();
    genotype_.reset();
    person_.reset();
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  void setup_person() {
    person_->set_age(25);
    person_->set_location(0);
    person_->set_residence_location(0);
    person_->set_host_state(Person::SUSCEPTIBLE);
    person_->set_innate_relative_biting_rate(1.0);
    person_->update_relative_biting_rate();
    person_->set_moving_level(1);
    person_->set_latest_update_time(0);
    person_->get_immune_system()->set_latest_immune_value(0.5);
    person_->generate_prob_present_at_mda_by_age();
  }

  std::unique_ptr<Person> person_;
  std::unique_ptr<Genotype> genotype_;
  std::unique_ptr<ClonalParasitePopulation> clinical_parasite_;
};

TEST_F(PersonRecrudescenceTreatmentFailureTest, HandlesTreatmentFailureDuringRecrudescence) {
  setup_person();
  clinical_parasite_->set_last_update_log10_parasite_density(3.0);

  constexpr int therapy_id = 1;
  const int initial_failures =
      Model::get_mdc()->number_of_treatments_fail_with_therapy_id()[therapy_id];
  auto failure_event = std::make_unique<TestTreatmentFailureEvent>(person_.get());
  failure_event->set_clinical_caused_parasite(clinical_parasite_.get());
  failure_event->set_therapy_id(therapy_id);
  failure_event->set_time(Model::get_scheduler()->current_time());
  auto* failure_event_ptr = failure_event.get();
  person_->schedule_basic_event(std::move(failure_event));

  person_->determine_symptomatic_recrudescence(clinical_parasite_.get());
  if (person_->get_recurrence_status() == Person::RecurrenceStatus::WITH_SYMPTOM) {
    EXPECT_FALSE(failure_event_ptr->is_executable());
    EXPECT_EQ(Model::get_mdc()->number_of_treatments_fail_with_therapy_id()[therapy_id],
              initial_failures + 1);
    bool found_recurrence = false;
    for (const auto &[time, scheduled_event] : person_->get_events()) {
      (void)time;
      auto* event = dynamic_cast<ProgressToClinicalEvent*>(scheduled_event.get());
      if (event != nullptr && event->clinical_caused_parasite() == clinical_parasite_.get()) {
        found_recurrence = true;
        break;
      }
    }
    EXPECT_TRUE(found_recurrence);
  } else {
    EXPECT_TRUE(failure_event_ptr->is_executable());
  }
}
