#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "Configuration/Config.h"
#include "Environment/SeasonalEquation.h"
#include "Simulation/Model.h"

namespace {
class SeasonalEquationRasterFixture : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->initialize();
    auto config = std::make_unique<Config>();
    config->get_spatial_settings().set_spatial_data(
        std::make_unique<SpatialData>(&config->get_spatial_settings()));
    Model::get_instance()->set_config(std::move(config));

    std::ofstream file("test_ecoclimatic.asc");
    file << "ncols 2\n"
         << "nrows 2\n"
         << "xllcorner 0\nyllcorner 0\ncellsize 1\nNODATA_value -9999\n"
         << "0 1\n9 -9999\n";
    file.close();
    Model::get_spatial_data()->load("test_ecoclimatic.asc", SpatialData::ECOCLIMATIC);
  }

  void TearDown() override {
    std::remove("test_ecoclimatic.asc");
    Model::get_instance()->release();
  }
};
}  // namespace

TEST_F(SeasonalEquationRasterFixture, LoadsValidZonesAndSkipsNodataAndUnknownZones) {
  SeasonalEquation equation;
  equation.set_raster_base({0.1, 0.2});
  equation.set_raster_A({0.3, 0.4});
  equation.set_raster_B({1.0, 2.0});
  equation.set_raster_phi({10, 20});
  equation.set_raster(true);

  auto *raster = Model::get_spatial_data()->get_raster(SpatialData::ECOCLIMATIC);
  ASSERT_NE(raster, nullptr);
  ASSERT_EQ(raster->nrows, 2);
  ASSERT_EQ(raster->ncols, 2);
  ASSERT_NO_THROW(equation.set_from_raster());
  ASSERT_EQ(equation.get_base().size(), 2U);
  EXPECT_DOUBLE_EQ(equation.get_base()[0], 0.1);
  EXPECT_DOUBLE_EQ(equation.get_base()[1], 0.2);
}
