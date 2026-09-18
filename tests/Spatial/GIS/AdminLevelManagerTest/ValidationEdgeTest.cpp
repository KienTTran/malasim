#include <gtest/gtest.h>

#include "Spatial/GIS/AdminLevelManager.h"

TEST(AdminLevelManagerValidationEdgeTest, RejectsInvalidIdsAndBoundaryRanges) {
  AdminLevelManager manager;
  const auto level = manager.register_level("district");
  EXPECT_THROW(manager.register_level("district"), std::runtime_error);
  EXPECT_THROW(manager.set_boundary(-1, {}), std::out_of_range);
  EXPECT_THROW(static_cast<void>(manager.get_admin_unit(-1, 0)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(manager.get_locations_in_unit(-1, 0)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(manager.get_unit_count(-1)), std::out_of_range);

  BoundaryData empty;
  EXPECT_THROW(manager.set_boundary(level, empty), std::runtime_error);

  BoundaryData invalid_min;
  invalid_min.min_unit_id = 2;
  invalid_min.max_unit_id = 2;
  invalid_min.unit_count = 1;
  EXPECT_THROW(manager.set_boundary(level, invalid_min), std::runtime_error);

  BoundaryData invalid_range;
  invalid_range.min_unit_id = 1;
  invalid_range.max_unit_id = 3;
  invalid_range.unit_count = 1;
  EXPECT_THROW(manager.set_boundary(level, invalid_range), std::runtime_error);
}

TEST(AdminLevelManagerValidationEdgeTest, RejectsMissingAndMalformedRasters) {
  AdminLevelManager manager;
  manager.register_level("district");
  EXPECT_THROW(manager.setup_boundary("missing", nullptr), std::runtime_error);
  EXPECT_THROW(manager.setup_boundary("district", nullptr), std::runtime_error);

  AscFile invalid;
  invalid.nrows = 0;
  invalid.ncols = 0;
  EXPECT_THROW(manager.setup_boundary("district", &invalid), std::runtime_error);

  AscFile non_contiguous;
  non_contiguous.nrows = 1;
  non_contiguous.ncols = 2;
  non_contiguous.nodata_value = -9999;
  non_contiguous.data = {{1.0F, 3.0F}};
  EXPECT_THROW(manager.setup_boundary("district", &non_contiguous), std::runtime_error);
}
