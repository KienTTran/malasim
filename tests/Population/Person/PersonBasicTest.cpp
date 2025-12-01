#include "PersonTestBase.h"

using namespace testing;

class PersonBasicTest : public PersonTestBase {
protected:
  void SetUp() override {
    PersonTestBase::SetUp();
    // If specific overrides are needed for this test suite,
    ON_CALL(*mock_population_, notify_change(_, _, _, _))
        .WillByDefault([](Person*, const Person::Property &, const void*,
                          const void*) { return; });
  }

  // Helper to calculate expected age class based on base fixture's demographic
  // config
  int get_expected_age_class(int age) const {
    int expected_class = 0;
    const auto &age_limits = mock_config_->get_population_demographic().get_age_structure();
    int num_classes = mock_config_->get_population_demographic().get_number_of_age_classes();
    while (expected_class < (num_classes - 1)
           && age >= age_limits[expected_class]) {
      expected_class++;
    }
    return expected_class;
  }
};

// Test to verify mock setup from the base class
TEST_F(PersonBasicTest, ConfigMockSetup) {
  // --- Verification of Setup State ---

  // Verify Model holds the correct mock pointer
  ASSERT_EQ(Model::get_config(), mock_config_)
      << "Model::get_config() does not return the expected mock_config_ "
         "pointer.";

  // Verify the mock returns the correct demographic object (by address)
  // This implicitly tests the ON_CALL for get_population_demographic() in SetUp
  const PopulationDemographic &returned_demographic =
      Model::get_config()->get_population_demographic();
  // Optionally, check a value within the returned demographic object
  ASSERT_EQ(returned_demographic.get_number_of_age_classes(), 6)
      << "Demographic object returned by get_population_demographic() has "
         "incorrect age class count.";

  // Verify the mock returns the correct number of age classes
  // This implicitly tests the ON_CALL for number_of_age_classes() in SetUp
  EXPECT_EQ(Model::get_config()->number_of_age_classes(), 6)
      << "Call to Model::get_config()->number_of_age_classes() did not return "
         "6.";

  // Verify the mock returns the correct age structure
  // This implicitly tests the ON_CALL for age_structure() in SetUp
  const auto &expected_age_structure = mock_config_->get_population_demographic().get_age_structure();
  const auto &actual_age_structure = Model::get_config()->age_structure();
  EXPECT_EQ(actual_age_structure, expected_age_structure)
      << "Call to Model::get_config()->age_structure() did not return the "
         "expected vector.";
  EXPECT_EQ(actual_age_structure.size(), 6)
      << "Age structure vector returned by "
         "Model::get_config()->age_structure() has incorrect size.";
}

TEST_F(PersonBasicTest, InitialState) {
  EXPECT_EQ(person_->get_host_state(), Person::HostStates::SUSCEPTIBLE);
  EXPECT_EQ(person_->get_age(), -1);
  EXPECT_EQ(person_->get_location(), -1);
  EXPECT_EQ(person_->get_residence_location(), -1);
}

// Test setting initial age and verifying class assignment
TEST_F(PersonBasicTest, AgeSetInitialClass) {

  // Expect notifications for both age and age class change (from 0 to 2)
  EXPECT_CALL(*mock_population_, notify_change(_, _, _, _))
      .Times(2);

  const int age_to_set = 25;
  const int expected_class = get_expected_age_class(age_to_set);
  EXPECT_EQ(expected_class, 2);  // Expect class index 2 for age 25

  person_->set_age(age_to_set);

  EXPECT_EQ(person_->get_age(), age_to_set);
  EXPECT_EQ(person_->get_age_class(), expected_class);
}

// Test age increase within the same class
TEST_F(PersonBasicTest, AgeIncrementStaysInClass) {
  // 1 for initial age change, 1 for age change after age set
  EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE, _, _))
      .Times(2);
  // only 1 for initial age class change
  EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE_CLASS, _, _))
      .Times(1);

  // Pre-condition: Set age to 25 (class 2)
  person_->set_age(25);
  ASSERT_EQ(person_->get_age_class(), 2);

  SCOPED_TRACE("Testing increase_age_by_1_year() from 25 to 26");
  const int current_age = 25;
  const int next_age = current_age + 1;
  const int expected_class = get_expected_age_class(next_age);
  EXPECT_EQ(expected_class, 2);  // Still expect class index 2 for age 26

  person_->increase_age_by_1_year();

  EXPECT_EQ(person_->get_age(), next_age);
  EXPECT_EQ(person_->get_age_class(), expected_class);
}

// Test age change that crosses a class boundary
TEST_F(PersonBasicTest, AgeSetCrossesClassBoundary) {
  // 1 for initial age change, 1 for age change after age set
  EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE, _, _))
      .Times(2);
  // 1 for initial age class change, 1 for age class change after age set
  EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE_CLASS, _, _))
      .Times(2);

  // Pre-condition: Set age to 26 (class 2)
  person_->set_age(26);
  ASSERT_EQ(person_->get_age_class(), 2);

  SCOPED_TRACE("Testing set_age(31)");
  const int age_to_set = 31;
  const int expected_class = get_expected_age_class(age_to_set);
  EXPECT_EQ(expected_class, 3);  // Expect class index 3 for age 31

  person_->set_age(age_to_set);

  EXPECT_EQ(person_->get_age(), age_to_set);
  EXPECT_EQ(person_->get_age_class(), expected_class);
}

// Test floating age calculation - Requires MockScheduler setup
TEST_F(PersonBasicTest, AgeFloatingCalculation) {
  // 1 for initial age change, 1 for age change after age set
  EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE, _, _))
      .Times(2);
  // 1 for initial age class change, 1 for age class change after age set
  EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE_CLASS, _, _))
      .Times(2);

  // Pre-condition: Set age to 26 (class 2)
  person_->set_age(26); 
  ASSERT_EQ(person_->get_age_class(), 2);

  const int current_sim_time =
      11415;  // Example time: 31 years + 100 days offset
  const int birthday_time = 100;

  // Set age and birthday - assume age is consistent with birthday and current
  // time
  person_->set_age(
      31);  // Set this just for consistency, not strictly needed for the calc
  person_->set_birthday(birthday_time);

  EXPECT_DOUBLE_EQ(person_->age_in_floating(current_sim_time), 31.0);
}

TEST_F(PersonBasicTest, LocationManagement) {
  // Test location setting
  EXPECT_CALL(*mock_population_,
              notify_change(_, Person::Property::LOCATION, _, _))
      .Times(1);

  person_->set_location(5);
  EXPECT_EQ(person_->get_location(), 5);

  // Test residence location
  person_->set_residence_location(3);
  EXPECT_EQ(person_->get_residence_location(), 3);
}

TEST_F(PersonBasicTest, HostStateTransitions) {
  // Test state transitions with notifications
  EXPECT_CALL(*mock_population_,
              notify_change(_, Person::Property::HOST_STATE, _, _))
      .Times(2);

  EXPECT_EQ(person_->get_host_state(), Person::HostStates::SUSCEPTIBLE);

  person_->set_host_state(Person::HostStates::EXPOSED);
  EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);

  person_->set_host_state(Person::HostStates::CLINICAL);
  EXPECT_EQ(person_->get_host_state(), Person::HostStates::CLINICAL);
}

TEST_F(PersonBasicTest, BitingRateManagement) {
  const double test_rate = 0.5;

  // Test innate biting rate
  person_->set_innate_relative_biting_rate(test_rate);
  EXPECT_DOUBLE_EQ(person_->get_innate_relative_biting_rate(), test_rate);

  // Test current biting rate
  person_->set_current_relative_biting_rate(test_rate * 2);
  EXPECT_DOUBLE_EQ(person_->get_current_relative_biting_rate(), test_rate * 2);
}

TEST_F(PersonBasicTest, MovingLevelManagement) {
  // Test moving level setting
  EXPECT_CALL(*mock_population_,
              notify_change(_, Person::Property::MOVING_LEVEL, _, _))
      .Times(1);

  person_->set_moving_level(2);
  EXPECT_EQ(person_->get_moving_level(), 2);
}

TEST_F(PersonBasicTest, UpdateTime) {
  const int test_time = 100;
  person_->set_latest_update_time(test_time);
  EXPECT_EQ(person_->get_latest_update_time(), test_time);
}

TEST_F(PersonBasicTest, RecurrenceStatus) {
  // Test status transitions
  EXPECT_EQ(person_->get_recurrence_status(), Person::RecurrenceStatus::NONE);

  person_->set_recurrence_status(Person::RecurrenceStatus::WITH_SYMPTOM);
  EXPECT_EQ(person_->get_recurrence_status(),
            Person::RecurrenceStatus::WITH_SYMPTOM);
}

