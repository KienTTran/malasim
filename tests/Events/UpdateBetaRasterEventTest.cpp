#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "Events/Population/UpdateBetaRasterEvent.hxx"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

namespace {
void write_beta_raster(const std::string &filename, int columns, int rows,
                       const std::vector<double> &values) {
  std::ofstream file(filename);
  ASSERT_TRUE(file.is_open());
  file << "ncols " << columns << "\n"
       << "nrows " << rows << "\n"
       << "xllcorner 0\n"
       << "yllcorner 0\n"
       << "cellsize 5000\n"
       << "NODATA_value -9999\n";
  for (int i = 0; i < columns * rows; ++i) {
    if (i != 0) file << ' ';
    file << values[static_cast<size_t>(i)];
    if ((i + 1) % columns == 0) file << '\n';
  }
}
}  // namespace

class UpdateBetaRasterEventTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override {
    Model::get_instance()->release();
    std::filesystem::remove("update_beta_test.asc");
    std::filesystem::remove("update_beta_overflow.asc");
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(UpdateBetaRasterEventTest, AppliesValuesAndSkipsNoDataCells) {
  write_beta_raster("update_beta_test.asc", 4, 2,
                   {0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4});
  UpdateBetaRasterEvent event("update_beta_test.asc", 0);
  event.set_executable(true);
  ASSERT_NO_THROW(event.execute());

  const auto &locations = Model::get_config()->location_db();
  ASSERT_EQ(locations.size(), 8U);
  EXPECT_NEAR(locations[0].beta, 0.7, 1e-6);
  EXPECT_NEAR(locations[1].beta, 0.8, 1e-6);
  EXPECT_NEAR(locations[2].beta, 0.9, 1e-6);
  EXPECT_NEAR(locations[3].beta, 1.0, 1e-6);
  EXPECT_NEAR(locations[4].beta, 1.1, 1e-6);
  EXPECT_NEAR(locations[5].beta, 1.2, 1e-6);
  EXPECT_NEAR(locations[6].beta, 1.3, 1e-6);
  EXPECT_NEAR(locations[7].beta, 1.4, 1e-6);
}

TEST_F(UpdateBetaRasterEventTest, RejectsMoreRasterCellsThanConfiguredLocations) {
  write_beta_raster("update_beta_overflow.asc", 9, 1,
                   {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9});
  UpdateBetaRasterEvent event("update_beta_overflow.asc", 0);
  event.set_executable(true);
  EXPECT_NO_THROW(event.execute());
  EXPECT_FALSE(event.is_executable());
}
