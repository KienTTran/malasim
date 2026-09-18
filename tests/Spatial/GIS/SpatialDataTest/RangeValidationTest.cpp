#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Spatial/GIS/SpatialData.h"

TEST(SpatialDataRangeValidationTest, NamesAndExpectedRangesCoverEveryRasterType) {
  const std::vector<std::pair<SpatialData::SpatialFileType, std::string>> names = {
      {SpatialData::LOCATIONS, "location"},
      {SpatialData::POPULATION, "population"},
      {SpatialData::BETA, "beta"},
      {SpatialData::DISTRICTS, "district"},
      {SpatialData::TRAVEL, "travel"},
      {SpatialData::ECOCLIMATIC, "ecoclimatic"},
      {SpatialData::PR_TREATMENT_UNDER5, "p_treatment_under_5"},
      {SpatialData::PR_TREATMENT_OVER5, "p_treatment_over_5"},
      {SpatialData::MOSQUITO_SIZE, "prmc_size"},
      {SpatialData::MOSQUITO_IFR, "interrupted_feeding_rate"},
      {SpatialData::COUNT, "unknown"},
  };

  for (const auto &[type, expected_name] : names) {
    EXPECT_EQ(SpatialData::get_type_name(type), expected_name);
  }

  for (const auto type : {SpatialData::PR_TREATMENT_UNDER5, SpatialData::PR_TREATMENT_OVER5,
                          SpatialData::MOSQUITO_IFR}) {
    const auto range = SpatialData::get_expected_value_range(type);
    ASSERT_TRUE(range.has_value());
    EXPECT_DOUBLE_EQ(range->minimum, 0.0);
    EXPECT_DOUBLE_EQ(range->maximum, 1.0);
  }

  for (const auto type : {SpatialData::LOCATIONS, SpatialData::POPULATION, SpatialData::BETA,
                          SpatialData::DISTRICTS, SpatialData::TRAVEL, SpatialData::ECOCLIMATIC,
                          SpatialData::MOSQUITO_SIZE}) {
    EXPECT_FALSE(SpatialData::get_expected_value_range(type).has_value());
  }
}

TEST(SpatialDataRangeValidationTest, IgnoresNullUnboundedAndNoDataCells) {
  EXPECT_NO_THROW(SpatialData::validate_value_range(nullptr, SpatialData::MOSQUITO_IFR, "null"));

  AscFile unbounded;
  unbounded.nrows = 1;
  unbounded.ncols = 1;
  unbounded.nodata_value = -9999;
  unbounded.data = {{9999.0F}};
  EXPECT_NO_THROW(
      SpatialData::validate_value_range(&unbounded, SpatialData::POPULATION, "population"));

  AscFile nodata;
  nodata.nrows = 1;
  nodata.ncols = 2;
  nodata.nodata_value = -9999;
  nodata.data = {{-9999.0F, 0.5F}};
  EXPECT_NO_THROW(
      SpatialData::validate_value_range(&nodata, SpatialData::MOSQUITO_IFR, "ifr"));
}

TEST(SpatialDataRangeValidationTest, RejectsOutOfRangeProbabilityCells) {
  AscFile raster;
  raster.nrows = 1;
  raster.ncols = 2;
  raster.nodata_value = -9999;
  raster.data = {{0.5F, 1.25F}};

  EXPECT_THROW(
      SpatialData::validate_value_range(&raster, SpatialData::PR_TREATMENT_UNDER5,
                                        "under5.asc"),
      std::runtime_error);
}
