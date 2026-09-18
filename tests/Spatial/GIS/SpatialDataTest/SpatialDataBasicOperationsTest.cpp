#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#define private public
#include "Spatial/GIS/SpatialData.h"
#undef private
#include "TestHelpers.h"

class SpatialDataBasicOperationsTest : public ::testing::Test, protected SpatialDataTestHelper {
protected:
  void SetUp() override { SpatialDataTestHelper::SetUp(); }
  void TearDown() override { SpatialDataTestHelper::TearDown(); }
};

TEST_F(SpatialDataBasicOperationsTest, VerifyDistrictRasterState) {
  ASSERT_TRUE(spatial_data->get_using_raster());
  EXPECT_EQ(spatial_data->get_boundary("district")->unit_count, 2);
}

TEST_F(SpatialDataBasicOperationsTest, VerifyPopulationRasterProperties) {
  ASSERT_TRUE(spatial_data->get_using_raster());
  auto header = spatial_data->get_raster_header();
  EXPECT_EQ(header.number_columns, 3);
  EXPECT_EQ(header.number_rows, 3);
  EXPECT_FLOAT_EQ(header.cellsize, 1.0f);
}

TEST_F(SpatialDataBasicOperationsTest, VerifyCompleteConfigurationState) {
  ASSERT_TRUE(spatial_data->get_using_raster());
  EXPECT_EQ(spatial_data->get_boundary("district")->unit_count, 2);
}

TEST_F(SpatialDataBasicOperationsTest, StoresRasterMetadataAccessors) {
  spatial_data->set_cell_size(2.5F);
  EXPECT_FLOAT_EQ(spatial_data->get_cell_size(), 2.5F);
  spatial_data->set_using_raster(false);
  EXPECT_FALSE(spatial_data->get_using_raster());

  SpatialData::RasterInformation info;
  info.number_columns = 7;
  spatial_data->set_raster_info(info);
  EXPECT_EQ(spatial_data->get_raster_info().number_columns, 7);
}

TEST_F(SpatialDataBasicOperationsTest, HandlesUnavailableAdministrativeManager) {
  spatial_data->admin_manager_.reset();
  EXPECT_TRUE(spatial_data->get_admin_levels().empty());
  EXPECT_FALSE(spatial_data->has_admin_level("district"));
  EXPECT_EQ(spatial_data->get_admin_units("district"), (std::pair<int, int>{-1, -1}));
}
