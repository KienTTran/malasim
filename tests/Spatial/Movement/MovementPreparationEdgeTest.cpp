#include <gtest/gtest.h>

#include "Simulation/Model.h"
#include "Spatial/Movement/MarshallSM.hxx"
#include "Spatial/Movement/WesolowskiSM.hxx"

TEST(MovementPreparationEdgeTest, PrepareReturnsSafelyWithoutModelConfiguration) {
  Model::get_instance()->release();

  Spatial::WesolowskiSM wesolowski(1.0, 0.1, 0.2, 0.3);
  Spatial::MarshallSM marshall(0.1, 0.2, 1.0, 2);
  EXPECT_NO_THROW(wesolowski.prepare());
  EXPECT_NO_THROW(marshall.prepare());
}

TEST(MovementPreparationEdgeTest, RejectsIncompleteCompatibilityDistanceRows) {
  Spatial::WesolowskiSM wesolowski(1.0, 0.1, 0.2, 0.3);
  Spatial::MarshallSM marshall(0.1, 0.2, 1.0, 2);
  const std::vector<int> residents = {100, 200};
  EXPECT_THROW(
      static_cast<void>(
          wesolowski.get_v_relative_out_movement_to_destination(0, 2, {1.0}, residents)),
      std::runtime_error);
  EXPECT_THROW(
      static_cast<void>(
          marshall.get_v_relative_out_movement_to_destination(0, 2, {1.0}, residents)),
      std::runtime_error);
}
