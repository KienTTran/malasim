#include "PersonTestBase.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Parasites/Genotype.h"
#include "Events/MoveParasiteToBloodEvent.h"
#include "Events/MatureGametocyteEvent.h"
#include "Events/UpdateWhenDrugIsPresentEvent.h"

class PersonParasiteTest : public PersonTestBase {
protected:
    int current_time_ = 30;
    void SetUp() override {
        PersonTestBase::SetUp();
        mock_scheduler_->set_current_time(current_time_);
        // Additional setup specific to parasite tests
    }
};

TEST_F(PersonParasiteTest, AddNewParasiteToBlood) {
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    
    auto parasite = person_->add_new_parasite_to_blood(genotype_ptr.get());
    EXPECT_NE(parasite, nullptr);    
    // Cleanup
}

TEST_F(PersonParasiteTest, InfectedByWhenLiverParasiteTypeIsNotSet) {
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(1);

    // Create Genotype using make_unique
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    // Get raw pointer for use in subsequent calls
    auto genotype_raw = genotype_ptr.get();

    // Add to database using std::move
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    // Test infection using the genotype ID (assuming this is correct usage)
    person_->infected_by(1);

    // Verify stage changed to Exposed
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);

    // check if just liver parasite type is set, using the raw pointer
    EXPECT_EQ(person_->liver_parasite_type(), genotype_raw);

    // a parasite is scheduled to be moved to blood in next 7 days
    EXPECT_EQ(person_->get_events().size(), 1);

    // cast to MoveParasiteToBloodEvent and check its genotype using the raw pointer
    auto event = dynamic_cast<MoveParasiteToBloodEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->infection_genotype(), genotype_raw);
    EXPECT_EQ(event->get_time(), current_time_ + 7);
}

TEST_F(PersonParasiteTest, InfectedByWhenLiverParasiteTypeIsSet) {
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(1);

    // First genotype
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));

    person_->infected_by(1);
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);
    EXPECT_EQ(person_->liver_parasite_type(), genotype_raw);

    // Second genotype
    auto new_genotype_ptr = std::make_unique<Genotype>("abcabcabc");
    new_genotype_ptr->set_genotype_id(2);
    // auto new_genotype_raw = new_genotype_ptr.get(); // Raw pointer not needed
    Model::get_genotype_db()->add(std::move(new_genotype_ptr));

    person_->infected_by(2);
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);

    // new genotype is not set to liver parasite type
    EXPECT_EQ(person_->liver_parasite_type(), genotype_raw);

    // a parasite is scheduled to be moved to blood in next 7 days by the old genotype
    EXPECT_EQ(person_->get_events().size(), 1);

    // cast to MoveParasiteToBloodEvent
    auto event = dynamic_cast<MoveParasiteToBloodEvent*>(person_->get_events().begin()->second.get());
    // infection genotype is the old genotype
    EXPECT_EQ(event->infection_genotype(), genotype_raw);
    EXPECT_EQ(event->get_time(), current_time_ + 7);
}

TEST_F(PersonParasiteTest, HasDetectableParasiteWhenNoParasiteInBlood) {
    bool has_detectable_parasite = person_->has_detectable_parasite();
    EXPECT_FALSE(has_detectable_parasite);
}

TEST_F(PersonParasiteTest, HasDetectableParasiteWhenParasiteInBlood) {
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    auto parasite = person_->add_new_parasite_to_blood(genotype_raw);
    parasite->set_last_update_log10_parasite_density(2.0);
    bool has_detectable_parasite = person_->has_detectable_parasite();
    EXPECT_TRUE(has_detectable_parasite);
}

TEST_F(PersonParasiteTest, HasParasiteInBloodsButUnderDetectionThreshold) {
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    auto parasite = person_->add_new_parasite_to_blood(genotype_raw);
    parasite->set_last_update_log10_parasite_density(-2.0);
    bool has_detectable_parasite = person_->has_detectable_parasite();
    EXPECT_FALSE(has_detectable_parasite);
}

TEST_F(PersonParasiteTest, IsGametocytaemicWhenParasiteInBloodAndCanProduceGametocytes) {
    // Initially should not be gametocytaemic
    EXPECT_FALSE(person_->is_gametocytaemic());

    // Add parasite and test again (implementation depends on how gametocytes are handled)
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    // have a parasite in blood, and parasite that can produce gametocytes
    auto parasite = person_->add_new_parasite_to_blood(genotype_raw);
    parasite->set_gametocyte_level(1.0);

    bool is_gametocytaemic = person_->is_gametocytaemic();
    EXPECT_TRUE(is_gametocytaemic);
}

TEST_F(PersonParasiteTest, NotGametocytaemicWhenNoParasiteInBlood) {
    EXPECT_FALSE(person_->is_gametocytaemic());
}

TEST_F(PersonParasiteTest, NotGametocytaemicWhenParasiteInBloodButNoGametocytes) {
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    auto parasite = person_->add_new_parasite_to_blood(genotype_raw);
    parasite->set_gametocyte_level(0.0);
    EXPECT_FALSE(person_->is_gametocytaemic());
}

// TODO: Implement this test later
TEST_F(PersonParasiteTest, RelativeInfectivity) {
    GTEST_SKIP() << "This test is marked for improvement later.";
    // Test relative infectivity calculation
    const double log10_parasite_density = 3.0;  // 1000 parasites
    double infectivity = Person::relative_infectivity(log10_parasite_density);
    
    EXPECT_GE(infectivity, 0.0);
    EXPECT_LE(infectivity, 1.0);
    /// make it fail
    EXPECT_FALSE(true);
}

TEST_F(PersonParasiteTest, ChangeStateWhenNoParasiteInBloodWhenNoParasiteInLiver) {
    // Test state change when no parasite in blood
    // there is 2 possible states, SUSCEPTIBLE and EXPOSED
    // if there is no parasite in liver, the state should be SUSCEPTIBLE
    // if there is a parasite in liver, the state should be EXPOSED

    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(2);

    // fake the state to be CLINICAL
    person_->set_host_state(Person::HostStates::CLINICAL);
    person_->change_state_when_no_parasite_in_blood();
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::SUSCEPTIBLE);
}

TEST_F(PersonParasiteTest, ChangeStateWhenNoParasiteInBloodWhenParasiteInLiver) {
    // Test state change when no parasite in blood
    // there is 2 possible states, SUSCEPTIBLE and EXPOSED
    // if there is no parasite in liver, the state should be SUSCEPTIBLE
    // if there is a parasite in liver, the state should be EXPOSED
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(2);
    
     // fake the state to be CLINICAL
    person_->set_host_state(Person::HostStates::CLINICAL);
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    person_->set_liver_parasite_type(genotype_raw);
    person_->change_state_when_no_parasite_in_blood();
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);
}

TEST_F(PersonParasiteTest, RandomlyChooseParasiteFrom2Infections) {
    EXPECT_CALL(*mock_random_, random_uniform(2)).WillOnce(::testing::Return(1));
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(1);

    // choose parasite from today infections
    auto genotype1_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype1_ptr->set_genotype_id(1);
    auto genotype2_ptr = std::make_unique<Genotype>("aaabbbccc"); // Should likely be different sequence
    genotype2_ptr->set_genotype_id(2);
    auto genotype2_raw = genotype2_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype1_ptr));
    Model::get_genotype_db()->add(std::move(genotype2_ptr));

    person_->get_today_infections().push_back(1);
    person_->get_today_infections().push_back(2);

    person_->randomly_choose_parasite();
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);
    EXPECT_EQ(person_->liver_parasite_type(), genotype2_raw);
    EXPECT_EQ(person_->get_today_infections().size(), 0);

    // should have 1 event scheduled
    EXPECT_EQ(person_->get_events().size(), 1);
    auto event = dynamic_cast<MoveParasiteToBloodEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->infection_genotype(), genotype2_raw);
    EXPECT_EQ(event->get_time(), current_time_ + 7);
}

TEST_F(PersonParasiteTest, RandomlyChooseParasiteFrom1Infection) {
    EXPECT_CALL(*mock_random_, random_uniform(1)).Times(0); // no neeed to randomly choose
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(1);

    // choose parasite from today infections
    auto genotype_ptr1 = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr1->set_genotype_id(1);
    auto genotype_raw1 = genotype_ptr1.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr1));
    person_->get_today_infections().push_back(1);
    person_->randomly_choose_parasite();
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);
    EXPECT_EQ(person_->liver_parasite_type(), genotype_raw1);
    EXPECT_EQ(person_->get_today_infections().size(), 0);

    // should have 1 event scheduled
    EXPECT_EQ(person_->get_events().size(), 1);
    auto event = dynamic_cast<MoveParasiteToBloodEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->infection_genotype(), genotype_raw1);
    EXPECT_EQ(event->get_time(), current_time_ + 7);
}

TEST_F(PersonParasiteTest, RandomlyChooseParasiteFrom0Infections) {
    EXPECT_CALL(*mock_random_, random_uniform(0)).Times(0); // no neeed to randomly choose
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(0);

    person_->randomly_choose_parasite();
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::SUSCEPTIBLE);
    EXPECT_EQ(person_->get_today_infections().size(), 0);
    EXPECT_EQ(person_->get_events().size(), 0);
}

TEST_F(PersonParasiteTest, RandomlyChooseParasiteFrom4Infections) {
    EXPECT_CALL(*mock_random_, random_uniform(4)).WillOnce(::testing::Return(2));
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::HOST_STATE, _, _)).Times(1);

    // choose parasite from today infections
    auto genotype_ptr1 = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr1->set_genotype_id(1);
    auto genotype_ptr2 = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr2->set_genotype_id(2);
    auto genotype_ptr3 = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr3->set_genotype_id(3);
    auto genotype_ptr4 = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr4->set_genotype_id(4);
    auto genotype_raw1 = genotype_ptr1.get();
    auto genotype_raw2 = genotype_ptr2.get();
    auto genotype_raw3 = genotype_ptr3.get();
    auto genotype_raw4 = genotype_ptr4.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr1));
    Model::get_genotype_db()->add(std::move(genotype_ptr2));
    Model::get_genotype_db()->add(std::move(genotype_ptr3));
    Model::get_genotype_db()->add(std::move(genotype_ptr4)); 

    person_->get_today_infections().push_back(1);
    person_->get_today_infections().push_back(2);
    person_->get_today_infections().push_back(3);
    person_->get_today_infections().push_back(4);

    person_->randomly_choose_parasite();
    EXPECT_EQ(person_->get_host_state(), Person::HostStates::EXPOSED);
    EXPECT_EQ(person_->liver_parasite_type(), genotype_raw3);
    EXPECT_EQ(person_->get_today_infections().size(), 0);

    // should have 1 event scheduled
    EXPECT_EQ(person_->get_events().size(), 1);
    auto event = dynamic_cast<MoveParasiteToBloodEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->infection_genotype(), genotype_raw3);
    EXPECT_EQ(event->get_time(), current_time_ + 7);
}


TEST_F(PersonParasiteTest, ScheduleMoveParasiteToBloodEvent) {
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    person_->schedule_move_parasite_to_blood(genotype_raw, 5);
    EXPECT_EQ(person_->get_events().size(), 1);

    auto event = dynamic_cast<MoveParasiteToBloodEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->infection_genotype(), genotype_raw);
    EXPECT_EQ(event->get_time(), current_time_ + 5);
}

TEST_F(PersonParasiteTest, ScheduleMatureGametocyteEventOver5) {
    // change age
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE, _, _)).Times(1);
    // change age class
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE_CLASS, _, _)).Times(1);

    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    
    auto parasite = person_->add_new_parasite_to_blood(genotype_raw);

    person_->set_age(10);
    
    person_->schedule_mature_gametocyte_event(parasite);
    EXPECT_EQ(person_->get_events().size(), 1);

    auto event = dynamic_cast<MatureGametocyteEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->blood_parasite(), parasite);
    EXPECT_EQ(event->get_time(), current_time_ + Model::get_config()->get_epidemiological_parameters().get_days_mature_gametocyte_over_five());
}

TEST_F(PersonParasiteTest, ScheduleMatureGametocyteEventUnder5) {
    // Set up expectations for age change notifications
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE, _, _));
    EXPECT_CALL(*mock_population_, notify_change(_, Person::Property::AGE_CLASS, _, _));

    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    
    auto parasite = person_->add_new_parasite_to_blood(genotype_raw);

    // Change age (this will trigger both AGE and AGE_CLASS notifications)
    person_->set_age(4);
    person_->schedule_mature_gametocyte_event(parasite);
    EXPECT_EQ(person_->get_events().size(), 1);

    auto event = dynamic_cast<MatureGametocyteEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->blood_parasite(), parasite);
    EXPECT_EQ(event->get_time(), current_time_ + Model::get_config()->get_epidemiological_parameters().get_days_mature_gametocyte_under_five());
}

TEST_F(PersonParasiteTest, ScheduleUpdateByDrugEvent) {
    auto genotype_ptr = std::make_unique<Genotype>("aaabbbccc");
    genotype_ptr->set_genotype_id(1);
    auto genotype_raw = genotype_ptr.get();
    Model::get_genotype_db()->add(std::move(genotype_ptr));
    auto parasite = person_->add_new_parasite_to_blood(genotype_raw);
    person_->schedule_update_by_drug_event(parasite);

    EXPECT_EQ(person_->get_events().size(), 1);
    auto event = dynamic_cast<UpdateWhenDrugIsPresentEvent*>(person_->get_events().begin()->second.get());
    EXPECT_EQ(event->clinical_caused_parasite(), parasite);
    EXPECT_EQ(event->get_time(), current_time_ + 1);
}

TEST_F(PersonParasiteTest, InfectionFromInfectiousBite) {
    // immune system will be
    EXPECT_CALL(*mock_immune_system_, get_current_value()).WillOnce(::testing::Return(0.5));

    // Setup mock expectations for infection probability
    // EXPECT_CALL(*mock_config_, get_p_infection_from_an_infectious_bite())
    //     .WillOnce(::testing::Return(0.5));
    
    double prob = person_->p_infection_from_an_infectious_bite();

    //(1 - immune_system_->get_current_value()) / 8.333 + 0.04;
    auto expected_prob = (1 - 0.5) / 8.333 + 0.04;
    EXPECT_DOUBLE_EQ(prob, expected_prob);
}

// TEST_F(PersonParasiteTest, ClearAllParasiteInfo) {
//     // Setup initial state with parasites
//     auto genotype1_ptr = std::make_unique<Genotype>("genotype1");
//     genotype1_ptr->set_genotype_id(1);
//     auto genotype1_raw = genotype1_ptr.get();
//     Model::get_genotype_db()->add(std::move(genotype1_ptr));

//     auto genotype2_ptr = std::make_unique<Genotype>("genotype2");
//     genotype2_ptr->set_genotype_id(2);
//     auto genotype2_raw = genotype2_ptr.get();
//     Model::get_genotype_db()->add(std::move(genotype2_ptr));

//     auto genotype3_ptr = std::make_unique<Genotype>("genotype3");
//     genotype3_ptr->set_genotype_id(3);
//     auto genotype3_raw = genotype3_ptr.get();
//     Model::get_genotype_db()->add(std::move(genotype3_ptr));

//     auto genotype4_ptr = std::make_unique<Genotype>("genotype4");
//     genotype4_ptr->set_genotype_id(4);
//     // auto genotype4_raw = genotype4_ptr.get(); // raw pointer not needed
//     Model::get_genotype_db()->add(std::move(genotype4_ptr));

//     person_->set_liver_parasite_type(genotype1_raw);
//     person_->add_new_parasite_to_blood(genotype2_raw);
//     person_->add_new_parasite_to_blood(genotype3_raw); // Add another blood parasite
//     person_->get_today_infections().push_back(4);

//     // Add some events (assuming MatureGametocyteEvent and UpdateWhenDrugIsPresentEvent exist)
//     MatureGametocyteEvent::schedule_event(mock_scheduler_.get(), person_, nullptr, 10);
//     UpdateWhenDrugIsPresentEvent::schedule_event(mock_scheduler_.get(), person_, nullptr, 15);

//     EXPECT_NE(person_->liver_parasite_type(), nullptr);
//     EXPECT_FALSE(person_->all_clonal_parasite_populations()->parasites()->empty());
//     EXPECT_FALSE(person_->get_today_infections().empty());
//     EXPECT_FALSE(person_->get_events().empty());

//     // Clear parasite info
//     person_->clear_all_parasite_info();

//     // Assertions after clearing
//     EXPECT_EQ(person_->liver_parasite_type(), nullptr);
//     EXPECT_TRUE(person_->all_clonal_parasite_populations()->parasites()->empty()); // Check if blood parasites are cleared
//     EXPECT_TRUE(person_->get_today_infections().empty());
//     EXPECT_TRUE(person_->get_events().empty()); // Assuming ClearAllParasiteInfo clears related events
// }

// TEST_F(PersonParasiteTest, ClearLatencyPeriodShouldDoNothingIfNoParasiteInLiver) {
//     EXPECT_EQ(person_->liver_parasite_type(), nullptr);
//     person_->clear_latency_period();
//     EXPECT_EQ(person_->liver_parasite_type(), nullptr); // Should remain null
//     EXPECT_TRUE(person_->get_events().empty()); // No events should be added or removed
// }


// TEST_F(PersonParasiteTest, ClearLatencyPeriodShouldClearParasiteAndEvent) {
//     // Setup: Person has a liver parasite and a MoveParasiteToBloodEvent scheduled
//     auto genotype_ptr = std::make_unique<Genotype>("genotype_liver");
//     genotype_ptr->set_genotype_id(1);
//     auto genotype_raw = genotype_ptr.get();
//     Model::get_genotype_db()->add(std::move(genotype_ptr));
//     person_->set_liver_parasite_type(genotype_raw);

//     // Manually schedule the event (assuming the event takes the scheduler, person, genotype, and time)
//     auto scheduled_time = mock_scheduler_->current_time() + 7; // Example time
//     MoveParasiteToBloodEvent::schedule_event(mock_scheduler_.get(), person_, genotype_raw, scheduled_time);

//     EXPECT_NE(person_->liver_parasite_type(), nullptr);
//     EXPECT_FALSE(person_->get_events().empty());

//     // Action: Clear latency period
//     person_->clear_latency_period();

//     // Assertions: Liver parasite and event should be cleared
//     EXPECT_EQ(person_->liver_parasite_type(), nullptr);
//     EXPECT_TRUE(person_->get_events().empty());
// }

// TEST_F(PersonParasiteTest, UpdateClearDrugEventsWhenNoDrug) {
//     // Setup: Person has some non-drug-related events
//     auto genotype_ptr = std::make_unique<Genotype>("genotype_event");
//     genotype_ptr->set_genotype_id(1);
//     auto genotype_raw = genotype_ptr.get();
//     Model::get_genotype_db()->add(std::move(genotype_ptr));
//     MatureGametocyteEvent::schedule_event(mock_scheduler_.get(), person_, genotype_raw, mock_scheduler_->current_time() + 5);

//     EXPECT_FALSE(person_->get_events().empty());
//     auto initial_event_count = person_->get_events().size();

//     // Action: Call update_clear_drug_events (assuming this is the intended method name)
//     // This method might not exist, adjust based on the actual class API.
//     // If it's update(), let's call that. Or maybe clear_event_by_class?
//     // Assuming a hypothetical clear_update_when_drug_present_events()
//     person_->clear_update_when_drug_present_events(); // Adjust name if needed

//     // Assertion: Non-drug events should remain
//     EXPECT_EQ(person_->get_events().size(), initial_event_count);
//     EXPECT_FALSE(person_->get_events().empty());
// }

// TEST_F(PersonParasiteTest, UpdateClearDrugEventsWhenDrugPresent) {
//     // Setup: Person has a drug-related event (UpdateWhenDrugIsPresentEvent)
//     auto genotype_ptr = std::make_unique<Genotype>("genotype_drug_event");
//     genotype_ptr->set_genotype_id(1);
//     auto genotype_raw = genotype_ptr.get(); // Needed if event scheduling uses it
//     Model::get_genotype_db()->add(std::move(genotype_ptr));
//     UpdateWhenDrugIsPresentEvent::schedule_event(mock_scheduler_.get(), person_, genotype_raw, mock_scheduler_->current_time() + 10);

//     EXPECT_FALSE(person_->get_events().empty());

//     // Action: Call method to clear drug events
//     person_->clear_update_when_drug_present_events(); // Adjust name if needed

//     // Assertion: The drug event should be cleared
//     EXPECT_TRUE(person_->get_events().empty());
// }

// TEST_F(PersonParasiteTest, RemoveParasite) {
//      auto genotype_ptr = std::make_unique<Genotype>("genotype_to_remove");
//      genotype_ptr->set_genotype_id(1);
//      auto genotype_raw = genotype_ptr.get();
//      Model::get_genotype_db()->add(std::move(genotype_ptr));

//      // Add the parasite to the person's blood
//      auto parasite_pop = person_->add_new_parasite_to_blood(genotype_raw);
//      EXPECT_FALSE(person_->all_clonal_parasite_populations()->parasites()->empty());

//      // Remove the parasite
//      person_->remove_parasite(parasite_pop);

//      // Assert the parasite is removed
//      EXPECT_TRUE(person_->all_clonal_parasite_populations()->parasites()->empty());
//      // Assert any related state changes or event removals if applicable
// } 