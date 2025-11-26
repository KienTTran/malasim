#include "PersonTestBase.h"
#include "Events/ReturnToResidenceEvent.h"
#include "Events/CirculateToTargetLocationNextDayEvent.h"


class PersonMovementTest : public PersonTestBase {
protected:
    void SetUp() override {
        PersonTestBase::SetUp();
        mock_scheduler_->set_current_time(15);
        // Additional setup specific to movement tests
    }
};

TEST_F(PersonMovementTest, RandomlyChooseTargetLocationWhenTodayTargetLocationsIsEmpty) {
    // this function is called to resolve and choose a target location
    // in today_target_locations_ vector
    
    // if today_target_locations_ is empty, then the person will not move
    EXPECT_EQ(person_->get_today_target_locations().size(), 0);

    person_->randomly_choose_target_location();

    // the person will not move , the choosing list is cleared and empty
    EXPECT_EQ(person_->get_today_target_locations().size(), 0);
    // no event is scheduled
    EXPECT_EQ(person_->get_events().size(), 0);
}

TEST_F(PersonMovementTest, RandomlyChooseTargetLocationWhenTodayTargetLocationsIsNotEmpty) {
    // if today_target_locations_ is not empty, then the person will move
    person_->get_today_target_locations().push_back(1);
    person_->randomly_choose_target_location();

    // the person will move to the target location and the choosing list is cleared
    EXPECT_EQ(person_->get_today_target_locations().size(), 0);
    // an event is scheduled
    EXPECT_EQ(person_->get_events().size(), 1);
    auto event = person_->get_events().begin()->second.get();
    EXPECT_EQ(event->get_time(), 15 + 1);

    // number_of_trips_taken_ is incremented
    EXPECT_EQ(person_->get_number_of_trips_taken(), 1);
}

TEST_F(PersonMovementTest, ReturnToResidenceEvents) {
    // Initially should have no return events
    EXPECT_FALSE(person_->has_return_to_residence_event());
    
    // Schedule a return event
    const int trip_length = 5;
    person_->schedule_return_to_residence_event(trip_length);
    
    // Verify event was scheduled
    EXPECT_TRUE(person_->has_return_to_residence_event());

    // the event is scheduled
    auto event = person_->get_events().begin()->second.get();
    EXPECT_EQ(event->get_time(), 15 + trip_length);
}

TEST_F(PersonMovementTest, MoveToTargetLocation) {
    const int target_location = 5;
    // Schedule move to target location
    person_->schedule_move_to_target_location_next_day_event(target_location);
    
    // Verify event was scheduled
    EXPECT_FALSE(person_->get_events().empty());

    // the event is scheduled, cast to CirculateToTargetLocationNextDayEvent
    auto event = dynamic_cast<CirculateToTargetLocationNextDayEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->get_time(), 15 + 1);
    // the person will move to the target location
    EXPECT_EQ(event->target_location(), target_location);
}

TEST_F(PersonMovementTest, TripTracking) {
    // Initially no trips
    EXPECT_EQ(person_->get_number_of_trips_taken(), 0);
    
    // Increment trips
    person_->set_number_of_trips_taken(1);
    EXPECT_EQ(person_->get_number_of_trips_taken(), 1);
}

TEST_F(PersonMovementTest, LocationStateNotification) {
  // Expect exactly one notification for a location change in this test
  EXPECT_CALL(*mock_population_,
              notify_change(_, Person::Property::LOCATION, _, _))
      .Times(1);

  const int new_location = 5;
  person_->set_location(new_location);
  EXPECT_EQ(person_->get_location(), new_location);
}


TEST_F(PersonMovementTest, MovingLevelNotification) {
    // Set initial moving level first without tracking notifications
    const int initial_level = 0;
    person_->set_moving_level(initial_level);
    EXPECT_EQ(person_->get_moving_level(), initial_level);
    
    // Now expect exactly one notification for the moving level change
    EXPECT_CALL(*mock_population_, 
               notify_change(_, Person::Property::MOVING_LEVEL, _, _))
        .Times(1);
    
    // Change moving level and verify notification worked
    const int new_level = 2;
    person_->set_moving_level(new_level);
    EXPECT_EQ(person_->get_moving_level(), new_level);
}

TEST_F(PersonMovementTest, ResidenceLocationManagement) {
    const int initial_residence = 0;
    const int new_residence = 3;
    
    // Set initial residence
    person_->set_residence_location(initial_residence);
    EXPECT_EQ(person_->get_residence_location(), initial_residence);
    
    // Change residence
    person_->set_residence_location(new_residence);
    EXPECT_EQ(person_->get_residence_location(), new_residence);
}

TEST_F(PersonMovementTest, TodayTargetLocations) {
    std::vector<int> test_locations = {1, 2, 3};
    
    // Add locations one by one
    for (int loc : test_locations) {
        person_->get_today_target_locations().push_back(loc);
    }
    
    // Verify locations were added
    const auto& target_locations = person_->get_today_target_locations();
    EXPECT_EQ(target_locations.size(), test_locations.size());
    for (size_t i = 0; i < test_locations.size(); ++i) {
        EXPECT_EQ(target_locations[i], test_locations[i]);
    }
}

#ifdef ENABLE_TRAVEL_TRACKING
TEST_F(PersonMovementTest, TravelTracking) {
    // Test travel tracking if enabled
    EXPECT_EQ(person_->get_day_that_last_trip_was_initiated(), -1);
    EXPECT_EQ(person_->get_day_that_last_trip_outside_district_was_initiated(), -1);
}
#endif 