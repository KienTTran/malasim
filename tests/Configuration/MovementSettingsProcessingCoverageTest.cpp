#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Configuration/MovementSettings.h"

TEST(MovementSettingsProcessingCoverageTest, BuildsEverySupportedSpatialMovementModel) {
  for (const std::string name : {"Barabasi", "Wesolowski", "WesolowskiSurface", "Marshall",
                                 "BurkinaFaso"}) {
    MovementSettings settings;
    MovementSettings::SpatialModelSettings spatial;
    spatial.set_name(name);
    settings.set_spatial_model_settings(spatial);

    ASSERT_NO_THROW(settings.process_config_using_spatial_settings(3));
    ASSERT_NE(settings.get_spatial_model(), nullptr);
    EXPECT_EQ(settings.get_v_moving_level_density().size(),
              static_cast<std::size_t>(settings.get_circulation_info().get_number_of_moving_levels()));
    EXPECT_EQ(settings.get_v_moving_level_value().size(),
              static_cast<std::size_t>(settings.get_circulation_info().get_number_of_moving_levels()));
    EXPECT_GT(settings.get_length_of_stay_theta(), 0.0);
    EXPECT_GT(settings.get_length_of_stay_k(), 0.0);
  }
}
