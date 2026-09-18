#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "Configuration/Config.h"
#include "Configuration/MosquitoParameters.h"
#include "Simulation/Model.h"

namespace {
void write_raster(const std::string &filename, const std::string &values) {
  std::ofstream file(filename);
  file << "ncols 2\n"
       << "nrows 1\n"
       << "xllcorner 0\n"
       << "yllcorner 0\n"
       << "cellsize 1\n"
       << "NODATA_value -9999\n"
       << values << "\n";
}

class GridModelFixture : public ::testing::Test {
protected:
  void SetUp() override {
    Model::get_instance()->initialize();
    auto config = std::make_unique<Config>();
    config->get_spatial_settings().set_spatial_data(
        std::make_unique<SpatialData>(&config->get_spatial_settings()));
    Model::get_instance()->set_config(std::move(config));
    write_raster("test_mosquito_size.asc", "80 120");
    write_raster("test_mosquito_ifr.asc", "0.2 0.4");
  }

  void TearDown() override {
    std::remove("test_mosquito_size.asc");
    std::remove("test_mosquito_ifr.asc");
    Model::get_instance()->release();
  }
};
}  // namespace

TEST_F(GridModelFixture, LoadsGridRastersAndAssignsValuesToLocations) {
  MosquitoParameters::GridBased grid;
  grid.set_prmc_size_raster("test_mosquito_size.asc");
  grid.set_interrupted_feeding_rate_raster("test_mosquito_ifr.asc");
  MosquitoParameters::MosquitoConfig config;
  config.set_mode("grid_based");
  config.set_grid_based(grid);

  MosquitoParameters parameters;
  parameters.set_mosquito_config(config);
  std::vector<Spatial::Location> locations(2);
  ASSERT_NO_THROW(parameters.process_config_using_locations(locations));
  EXPECT_EQ(locations[0].mosquito_size, 80);
  EXPECT_EQ(locations[1].mosquito_size, 120);
  EXPECT_FLOAT_EQ(locations[0].mosquito_ifr, 0.2F);
  EXPECT_FLOAT_EQ(locations[1].mosquito_ifr, 0.4F);
}

TEST_F(GridModelFixture, RejectsGridProcessingWhenIfrRasterIsAbsent) {
  MosquitoParameters::GridBased grid;
  grid.set_prmc_size_raster("test_mosquito_size.asc");
  MosquitoParameters::MosquitoConfig config;
  config.set_mode("grid_based");
  config.set_grid_based(grid);

  MosquitoParameters parameters;
  parameters.set_mosquito_config(config);
  std::vector<Spatial::Location> locations(2);
  EXPECT_THROW(parameters.process_config_using_locations(locations), std::invalid_argument);
}
