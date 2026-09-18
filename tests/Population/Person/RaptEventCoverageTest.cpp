#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "Events/RaptEvent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class RaptEventCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &config) {
      config["rapt_settings"]["enabled"] = true;
      config["rapt_settings"]["age_start"] = 0;
      config["rapt_settings"]["compliance"] = 1.0;
      config["rapt_settings"]["period"] = 12;
      config["rapt_settings"]["start_date"] = "2000/1/1";
      config["spatial_settings"]["grid_based"]["p_treatment_under_5_raster"] =
          "rapt_treatment.asc";
      config["spatial_settings"]["grid_based"]["p_treatment_over_5_raster"] =
          "rapt_treatment.asc";
      test_fixtures::create_test_raster_file("rapt_treatment.asc", 10, 10, 1.0);
    });
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());

    person_ = std::make_unique<Person>();
    person_->initialize();
    person_->set_location(0);
    person_->set_residence_location(0);
    person_->set_host_state(Person::SUSCEPTIBLE);
    Model::get_scheduler()->set_current_time(0);

  }

  void TearDown() override {
    person_.reset();
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<Person> person_;
};

TEST_F(RaptEventCoverageTest, AppliesConfiguredTreatmentRateToUnderFivePerson) {
  person_->set_age(2);
  RaptEvent event(person_.get());
  event.set_executable(true);

  ASSERT_NO_THROW(event.execute());
  EXPECT_FALSE(event.is_executable());
  EXPECT_FALSE(person_->get_events().empty());
}

TEST_F(RaptEventCoverageTest, AppliesConfiguredTreatmentRateToOverFivePerson) {
  person_->set_age(20);
  RaptEvent event(person_.get());
  event.set_executable(true);

  ASSERT_NO_THROW(event.execute());
  EXPECT_FALSE(event.is_executable());
  EXPECT_FALSE(person_->get_events().empty());
}
