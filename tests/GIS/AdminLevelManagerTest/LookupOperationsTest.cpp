#include <gtest/gtest.h>

#include "Spatial/GIS/AscFile.h"
#include "TestHelpers.h"

class LookupOperationsTest : public AdminLevelManagerTestHelper, public ::testing::Test {
protected:
  void SetUp() override {
    AdminLevelManagerTestHelper::SetUp();
    setup_test_boundaries();
  }

  void TearDown() override { AdminLevelManagerTestHelper::TearDown(); }

  void setup_test_boundaries() {
    // Create a simple district raster with known values
    std::vector<std::vector<int>> district_values = {{1, 1, 2}, {1, 1, 2}, {3, 3, 3}};
    create_custom_raster("test_district.asc", district_values);

    Model::get_spatial_data()->reset_raster_info();
    Model::get_spatial_data()->load("test_district.asc", SpatialData::DISTRICTS);
    Model::get_spatial_data()->set_using_raster(true);
    // Check to make sure our data is OK
    std::string errors;
    if (Model::get_spatial_data()->check_catalog(errors)) { throw std::runtime_error(errors); }
    auto district_raster = Model::get_spatial_data()->get_raster(SpatialData::DISTRICTS);
    Model::get_spatial_data()->generate_locations(district_raster);
    Model::get_spatial_data()->parse_complete();
    auto raster = std::make_unique<AscFile>(*district_raster);
    EXPECT_NO_THROW(manager.register_level("district"));
    EXPECT_NO_THROW(manager.setup_boundary("district", std::move(raster).get()));
    raster.reset();
  }
};

TEST_F(LookupOperationsTest, GetAdminUnitForLocation) {
  // Test various locations based on the known raster pattern
  EXPECT_EQ(manager.get_admin_unit("district", 0), 1);  // First district
  EXPECT_EQ(manager.get_admin_unit("district", 4), 1);  // Second district
  EXPECT_EQ(manager.get_admin_unit("district", 2), 2);  // Second district
  EXPECT_EQ(manager.get_admin_unit("district", 8), 3);  // Third district
}

TEST_F(LookupOperationsTest, InvalidLocationHandling) {
  EXPECT_THROW(
    {
      auto admin_unit = manager.get_admin_unit("district", -1);
    }, std::out_of_range);
  EXPECT_THROW(
    {
      auto admin_unit = manager.get_admin_unit("district", 999);
    }, std::out_of_range);
}

TEST_F(LookupOperationsTest, GetLocationsInAdminUnit) {
  auto locations_d1 = manager.get_locations_in_unit("district", 1);
  EXPECT_EQ(locations_d1.size(), 4);  // District 1 has 4 cells

  auto locations_d2 = manager.get_locations_in_unit("district", 2);
  EXPECT_EQ(locations_d2.size(), 2);  // District 2 has 2 cells

  auto locations_d3 = manager.get_locations_in_unit("district", 3);
  EXPECT_EQ(locations_d3.size(), 3);  // District 3 has 3 cells
}

TEST_F(LookupOperationsTest, InvalidAdminUnitHandling) {
  EXPECT_THROW(
    {
      auto locations = manager.get_locations_in_unit("district", 999);
    }, std::out_of_range);
}

TEST_F(LookupOperationsTest, GetBoundaryData) {
  const auto* boundary = manager.get_boundary("district");
  ASSERT_NE(boundary, nullptr);
  EXPECT_EQ(boundary->unit_count, 3);  // Three unique districts
}

TEST_F(LookupOperationsTest, GetUnitCount) { EXPECT_EQ(manager.get_unit_count("district"), 3); }

TEST_F(LookupOperationsTest, InvalidBoundaryAccess) {
  EXPECT_EQ(manager.get_boundary("nonexistent"), nullptr);
  EXPECT_THROW(
    {
      auto unit_count = manager.get_unit_count("nonexistent");
    }, std::runtime_error);
}

