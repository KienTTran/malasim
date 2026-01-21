#include <gtest/gtest.h>
#include <memory>

#include "Treatment/Therapies/MACTherapy.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"

class MACTherapyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
    
    // Create a MAC therapy
    mac_therapy = std::make_unique<MACTherapy>();
    mac_therapy->set_id(1);
    mac_therapy->set_name("MAC Therapy Test");
  }

  void TearDown() override {
    mac_therapy.reset();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<MACTherapy> mac_therapy;
};

TEST_F(MACTherapyTest, Initialization) {
  EXPECT_EQ(mac_therapy->get_id(), 1);
  EXPECT_EQ(mac_therapy->get_name(), "MAC Therapy Test");
  EXPECT_TRUE(mac_therapy->get_therapy_ids().empty());
  EXPECT_TRUE(mac_therapy->get_start_at_days().empty());
}

TEST_F(MACTherapyTest, AddTherapyId) {
  mac_therapy->add_therapy_id(101);
  
  ASSERT_EQ(mac_therapy->get_therapy_ids().size(), 1);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[0], 101);
  
  mac_therapy->add_therapy_id(102);
  
  ASSERT_EQ(mac_therapy->get_therapy_ids().size(), 2);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[0], 101);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[1], 102);
}

TEST_F(MACTherapyTest, AddSchedule) {
  mac_therapy->add_schedule(5);
  
  ASSERT_EQ(mac_therapy->get_start_at_days().size(), 1);
  EXPECT_EQ(mac_therapy->get_start_at_days()[0], 5);
  
  mac_therapy->add_schedule(10);
  
  ASSERT_EQ(mac_therapy->get_start_at_days().size(), 2);
  EXPECT_EQ(mac_therapy->get_start_at_days()[0], 5);
  EXPECT_EQ(mac_therapy->get_start_at_days()[1], 10);
}

TEST_F(MACTherapyTest, SetTherapyIds) {
  std::vector<int> therapy_ids = {201, 202, 203};
  mac_therapy->set_therapy_ids(therapy_ids);
  
  ASSERT_EQ(mac_therapy->get_therapy_ids().size(), 3);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[0], 201);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[1], 202);
  EXPECT_EQ(mac_therapy->get_therapy_ids()[2], 203);
}

TEST_F(MACTherapyTest, SetStartAtDays) {
  std::vector<int> start_days = {1, 3, 7};
  mac_therapy->set_start_at_days(start_days);
  
  ASSERT_EQ(mac_therapy->get_start_at_days().size(), 3);
  EXPECT_EQ(mac_therapy->get_start_at_days()[0], 1);
  EXPECT_EQ(mac_therapy->get_start_at_days()[1], 3);
  EXPECT_EQ(mac_therapy->get_start_at_days()[2], 7);
}
