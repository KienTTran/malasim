#include "EventManagerTestCommon.h"

class EdgeCasesTest : public EventManagerTestBase {};

TEST_F(EdgeCasesTest, ScheduleNullEvent) {
    // Scheduling nullptr should be safe and not affect the event queue
    event_manager.schedule_event(nullptr);
    EXPECT_TRUE(event_manager.get_events().empty());
    
    // Execute events to ensure no crashes
    event_manager.execute_events(10);
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, CancelNullEvent) {
    // Cancelling nullptr should be safe and not affect the event queue
    event_manager.cancel_event(nullptr);
    EXPECT_TRUE(event_manager.get_events().empty());
    
    // Execute events to ensure no crashes
    event_manager.execute_events(10);
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, ExecuteEventsAtZeroTime) {
    auto* event = new testing::StrictMock<MockEvent>(0);
    
    EXPECT_CALL(*event, do_execute()).Times(1);
    EXPECT_CALL(*event, die()).Times(1);
    
    event_manager.schedule_event(event);
    event_manager.execute_events(0);
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, ExecuteEventsAtNegativeTime) {
    auto* event1 = new testing::StrictMock<MockEvent>(-5);
    auto* event2 = new testing::StrictMock<MockEvent>(-10);
    
    {
        testing::InSequence seq;
        EXPECT_CALL(*event2, do_execute()).Times(1);  // Earlier negative time first
        EXPECT_CALL(*event2, die()).Times(1);
        EXPECT_CALL(*event1, do_execute()).Times(1);  // Later negative time second
        EXPECT_CALL(*event1, die()).Times(1);
    }
    
    event_manager.schedule_event(event1);
    event_manager.schedule_event(event2);
    event_manager.execute_events(0);
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, ExecuteEventsAtMaxTime) {
    auto* event = new testing::StrictMock<MockEvent>(std::numeric_limits<int>::max());
    
    EXPECT_CALL(*event, do_execute()).Times(1);
    EXPECT_CALL(*event, die()).Times(1);
    
    event_manager.schedule_event(event);
    event_manager.execute_events(std::numeric_limits<int>::max());
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, RescheduleCancelledEvent) {
    auto* event = new testing::StrictMock<MockEvent>(10);
    
    // First schedule and cancel
    EXPECT_CALL(*event, do_execute()).Times(0);
    EXPECT_CALL(*event, die()).Times(1);
    
    event_manager.schedule_event(event);
    event_manager.cancel_event(event);
    
    // Create and schedule a new event
    auto* new_event = new testing::StrictMock<MockEvent>(15);
    EXPECT_CALL(*new_event, do_execute()).Times(1);
    EXPECT_CALL(*new_event, die()).Times(1);
    
    event_manager.schedule_event(new_event);
    event_manager.execute_events(15);
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, ExecuteEventMultipleTimes) {
    auto* event = new testing::StrictMock<MockEvent>(10);
    
    EXPECT_CALL(*event, do_execute()).Times(1);  // Should only execute once
    EXPECT_CALL(*event, die()).Times(1);
    
    event_manager.schedule_event(event);
    
    // Execute multiple times at the same time point
    event_manager.execute_events(10);
    event_manager.execute_events(10);
    event_manager.execute_events(10);
    
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, CancelEventDuringExecution) {
    class SelfCancellingEvent : public PersonEvent {
    public:
        explicit SelfCancellingEvent(int time, EventManager<PersonEvent>& mgr) 
            : PersonEvent(nullptr), manager(mgr) { set_time(time); }
        
        MOCK_METHOD(void, do_execute, (), (override));
        MOCK_METHOD(const std::string, name, (), (const, override));
        MOCK_METHOD(void, die, ());  // Helper method to track destruction
    
        ~SelfCancellingEvent() override {
            die();  // Call mock method in destructor
        }

        void execute()  {
            do_execute();
            // Cancel itself during execution
            // this simply marks the event as non-executable but does not remove it
            // but after execution, it will be removed from the event queue
            manager.cancel_event(this);  
        }
        
    private:
        EventManager<PersonEvent>& manager;
    };
    
    auto* event = new testing::StrictMock<SelfCancellingEvent>(10, event_manager);
    
    EXPECT_CALL(*event, do_execute()).Times(1);
    EXPECT_CALL(*event, die()).Times(1);
    event_manager.schedule_event(event);
    event_manager.execute_events(10);
    EXPECT_TRUE(event_manager.get_events().empty());
}

TEST_F(EdgeCasesTest, HasEventOfTypeWithEmptyQueue) {
    EXPECT_FALSE(event_manager.has_event<MockEvent>());
    
    auto* event = new testing::StrictMock<MockEvent>(10);
    EXPECT_CALL(*event, do_execute()).Times(1);
    EXPECT_CALL(*event, die()).Times(1);
    
    event_manager.schedule_event(event);
    EXPECT_TRUE(event_manager.has_event<MockEvent>());
    
    event_manager.execute_events(10);
    EXPECT_FALSE(event_manager.has_event<MockEvent>());
} 