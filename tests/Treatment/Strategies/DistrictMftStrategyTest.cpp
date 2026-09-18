#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Spatial/GIS/SpatialData.h"
#include "Treatment/Strategies/DistrictMftStrategy.h"
#include "Treatment/Therapies/Therapy.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class DistrictMftStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    Model::get_instance()->release();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());

    // Create strategy
    strategy = std::make_unique<DistrictMftStrategy>();
    strategy->id = 1;
    strategy->name = "TestDistrictMftStrategy";

    // Use therapies from the therapy database instead of creating new ones.
    ASSERT_GE(Model::get_therapy_db().size(), 3U)
        << "The generated fixture must provide at least three therapies";
    therapy1 = Model::get_therapy_db()[0].get();
    therapy2 = Model::get_therapy_db()[1].get();
    therapy3 = Model::get_therapy_db()[2].get();

    // The generated district raster assigns locations 0-3 to district 1 and
    // locations 4-5 to district 2.
    person1 = std::make_unique<Person>();
    person1->set_location(0);

    person2 = std::make_unique<Person>();
    person2->set_location(4);
  }

  void TearDown() override {
    person2.reset();
    person1.reset();
    strategy.reset();
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  // Helper to create MFT strategy for a district
  std::unique_ptr<DistrictMftStrategy::MftStrategy> create_district_strategy(
      const std::vector<int> &therapy_ids, const std::vector<double> &percentages) {
    auto dist_strategy = std::make_unique<DistrictMftStrategy::MftStrategy>();
    dist_strategy->therapies = therapy_ids;
    dist_strategy->percentages = percentages;
    return dist_strategy;
  }

  std::unique_ptr<DistrictMftStrategy> strategy;
  Therapy* therapy1;
  Therapy* therapy2;
  Therapy* therapy3;
  std::unique_ptr<Person> person1;
  std::unique_ptr<Person> person2;
};

TEST_F(DistrictMftStrategyTest, Initialization) {
  EXPECT_EQ(strategy->id, 1);
  EXPECT_EQ(strategy->name, "TestDistrictMftStrategy");
  EXPECT_EQ(strategy->get_type(), IStrategy::StrategyType::DistrictMft);
}

TEST_F(DistrictMftStrategyTest, AddTherapyThrowsException) {
  // Adding therapies directly should throw an exception
  EXPECT_THROW(strategy->add_therapy(therapy1), std::runtime_error);
}

TEST_F(DistrictMftStrategyTest, SetDistrictStrategy) {
  // Create district strategy 1
  std::vector<int> therapy_ids1 = {therapy1->get_id(), therapy2->get_id()};
  std::vector<double> percentages1 = {0.7, 0.3};
  auto district_strategy1 = create_district_strategy(therapy_ids1, percentages1);

  // Set district strategy for district 0
  // Note: Since this is dependent on SpatialData initialization,
  // we'll use district IDs that should be valid in most configurations
  EXPECT_NO_THROW(strategy->set_district_strategy(0, std::move(district_strategy1)));

  // Create district strategy 2
  std::vector<int> therapy_ids2 = {therapy2->get_id(), therapy3->get_id()};
  std::vector<double> percentages2 = {0.4, 0.6};
  auto district_strategy2 = create_district_strategy(therapy_ids2, percentages2);

  // Set district strategy for district 1
  EXPECT_NO_THROW(strategy->set_district_strategy(1, std::move(district_strategy2)));
}

TEST_F(DistrictMftStrategyTest, SetDistrictStrategyTwiceThrowsException) {
  // Create district strategy
  std::vector<int> therapy_ids = {therapy1->get_id(), therapy2->get_id()};
  std::vector<double> percentages = {0.7, 0.3};
  auto district_strategy1 = create_district_strategy(therapy_ids, percentages);

  // Set district strategy for district 0
  strategy->set_district_strategy(0, std::move(district_strategy1));

  // Try to set it again, should throw
  auto district_strategy2 = create_district_strategy(therapy_ids, percentages);
  EXPECT_THROW(strategy->set_district_strategy(0, std::move(district_strategy2)),
               std::runtime_error);
}

TEST_F(DistrictMftStrategyTest, SetInvalidDistrictThrowsException) {
  // Create district strategy
  std::vector<int> therapy_ids = {therapy1->get_id(), therapy2->get_id()};
  std::vector<double> percentages = {0.7, 0.3};
  auto district_strategy = create_district_strategy(therapy_ids, percentages);

  // Try to set for an invalid district (negative ID)
  EXPECT_THROW(strategy->set_district_strategy(-1, std::move(district_strategy)),
               std::out_of_range);
}

// NOTE: The following test depends on SpatialData's mapping of location to districts
// This may be hard to fully test without mocking SpatialData, but we'll structure
// it to be as robust as possible
TEST_F(DistrictMftStrategyTest, GetTherapyForPerson) {
  // Since get_therapy depends on SpatialData's mapping of location to district,
  // and Model::get_random() for probability generation, this test may be brittle
  // We'll do a basic test to ensure it doesn't crash, with the caveat that
  // the actual therapy selection depends on runtime configuration

  // Set up district strategies first
  // District 0 strategy
  std::vector<int> therapy_ids0 = {therapy1->get_id(), therapy2->get_id()};
  std::vector<double> percentages0 = {1.0, 0.0};  // Always select therapy1 for determinism
  auto district_strategy0 = create_district_strategy(therapy_ids0, percentages0);
  strategy->set_district_strategy(1, std::move(district_strategy0));

  // District 1 strategy
  std::vector<int> therapy_ids1 = {therapy2->get_id(), therapy3->get_id()};
  std::vector<double> percentages1 = {1.0, 0.0};  // Always select therapy2 for determinism
  auto district_strategy1 = create_district_strategy(therapy_ids1, percentages1);
  strategy->set_district_strategy(2, std::move(district_strategy1));

  // Try to get therapies for persons
  // This may fail if SpatialData maps locations to different districts than expected
  // We're just checking that the method executes without exception
  EXPECT_NO_THROW(strategy->get_therapy(person1.get()));
  EXPECT_NO_THROW(strategy->get_therapy(person2.get()));
}

TEST_F(DistrictMftStrategyTest, ToStringReturnsName) {
  // The to_string method just returns the name
  EXPECT_EQ(strategy->to_string(), "TestDistrictMftStrategy");
}

TEST_F(DistrictMftStrategyTest, LifecycleMethods) {
  // These methods are empty in DistrictMftStrategy but should not crash
  EXPECT_NO_THROW(strategy->update_end_of_time_step());
  EXPECT_NO_THROW(strategy->adjust_started_time_point(100));
  EXPECT_NO_THROW(strategy->monthly_update());
}

TEST_F(DistrictMftStrategyTest, GetTherapyThrowsForUnassignedDistrict) {
  // No district has been given an MFT in this test, so the lookup must report
  // the problem rather than dereferencing a null entry. The previous
  // implementation indexed the map with operator[], which inserts a null entry
  // for an unexpected id and then dereferences it.
  const auto district =
      Model::get_spatial_data()->get_admin_unit("district", person1->get_location());

  try {
    strategy->get_therapy(person1.get());
    FAIL() << "Expected std::runtime_error for a district with no MFT assigned";
  } catch (const std::runtime_error &ex) {
    EXPECT_NE(std::string(ex.what()).find(std::to_string(district)), std::string::npos)
        << "message should name the offending district: " << ex.what();
  }
}

TEST_F(DistrictMftStrategyTest, GetTherapyDoesNotThrowWhenDistributionUnderflows) {
  // The cumulative scan can run past the end of the distribution when the
  // shares accumulate to slightly less than 1.0. That used to throw and abort
  // the whole simulation. With float shares the residual was around 1.5e-8, so
  // over the hundreds of millions of treatments in a national-scale run this
  // was a live crash risk rather than a theoretical one.
  //
  // A distribution summing to 0.5 exaggerates the same condition so it fires on
  // roughly half of the draws instead of one in 10^8.
  const auto district =
      Model::get_spatial_data()->get_admin_unit("district", person1->get_location());

  std::vector<int> therapy_ids = {therapy1->get_id(), therapy2->get_id()};
  std::vector<double> percentages = {0.25, 0.25};
  strategy->set_district_strategy(district, create_district_strategy(therapy_ids, percentages));

  for (int i = 0; i < 1000; i++) {
    Therapy* selected = nullptr;
    ASSERT_NO_THROW(selected = strategy->get_therapy(person1.get()));
    ASSERT_NE(selected, nullptr);
    EXPECT_TRUE(selected->get_id() == therapy1->get_id()
                || selected->get_id() == therapy2->get_id())
        << "selection fell outside the configured therapy list";
  }
}

TEST_F(DistrictMftStrategyTest, GetTherapyRespectsDistrictSpecificDistribution) {
  // A deterministic distribution proves the per-district lookup actually
  // resolves through SpatialData rather than returning an arbitrary therapy.
  const auto district =
      Model::get_spatial_data()->get_admin_unit("district", person1->get_location());

  std::vector<int> therapy_ids = {therapy1->get_id(), therapy2->get_id()};
  std::vector<double> percentages = {1.0, 0.0};
  strategy->set_district_strategy(district, create_district_strategy(therapy_ids, percentages));

  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(strategy->get_therapy(person1.get())->get_id(), therapy1->get_id());
  }
}

TEST_F(DistrictMftStrategyTest, GetTherapyToleratesRepeatedLookups) {
  // The lookup path was changed from a std::map keyed by district id plus a
  // string-keyed admin level lookup to two direct vector indexes. Hammer it to
  // confirm the cached level id resolves the same district every time.
  const auto district =
      Model::get_spatial_data()->get_admin_unit("district", person1->get_location());

  std::vector<int> therapy_ids = {therapy1->get_id(), therapy2->get_id()};
  std::vector<double> percentages = {1.0, 0.0};
  strategy->set_district_strategy(district, create_district_strategy(therapy_ids, percentages));

  for (int i = 0; i < 10000; i++) {
    ASSERT_EQ(strategy->get_therapy(person1.get())->get_id(), therapy1->get_id());
  }
}

TEST_F(DistrictMftStrategyTest, DistinctDistrictsKeepDistinctStrategies) {
  // Guards against the vector being sized or indexed incorrectly after the
  // switch away from std::map.
  const auto district1 =
      Model::get_spatial_data()->get_admin_unit("district", person1->get_location());
  const auto district2 =
      Model::get_spatial_data()->get_admin_unit("district", person2->get_location());

  ASSERT_NE(district1, district2);

  strategy->set_district_strategy(
      district1, create_district_strategy({therapy1->get_id(), therapy2->get_id()}, {1.0, 0.0}));
  strategy->set_district_strategy(
      district2, create_district_strategy({therapy2->get_id(), therapy3->get_id()}, {1.0, 0.0}));

  EXPECT_EQ(strategy->get_therapy(person1.get())->get_id(), therapy1->get_id());
  EXPECT_EQ(strategy->get_therapy(person2.get())->get_id(), therapy2->get_id());
}
