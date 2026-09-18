#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

#include "Spatial/GIS/SpatialData.h"
#include "Spatial/GIS/AscFile.h"
#include "TestHelpers.h"

class ErrorHandlingTest : public ::testing::Test, protected SpatialDataTestHelper {
protected:
  void SetUp() override { SpatialDataTestHelper::SetUp(); }

  void TearDown() override { SpatialDataTestHelper::TearDown(); }

  void create_mismatched_raster(const std::string &filename) {
    std::ofstream file(filename);
    file << "ncols 4\n";
    file << "nrows 4\n";
    file << "xllcorner 0.0\n";
    file << "yllcorner 0.0\n";
    file << "cellsize 1.0\n";
    file << "NODATA_value -9999\n";
    file << "1 1 1 1\n";
    file << "1 1 1 1\n";
    file << "1 1 1 1\n";
    file << "1 1 1 1\n";
    file.close();
  }

  void create_invalid_district_raster(const std::string &filename) {
    std::ofstream file(filename);
    file << "ncols 3\nnrows 3\nxllcorner 0.0\nyllcorner 0.0\ncellsize 1.0\nNODATA_value -9999\n";
    file << "1 1 3\n";
    file << "1 3 3\n";
    file << "-9999 3 3\n";
    file.close();
  }

  void create_excess_value_raster(const std::string &filename) {
    std::ofstream file(filename);
    file << "ncols 3\nnrows 3\nxllcorner 0.0\nyllcorner 0.0\ncellsize 1.0\nNODATA_value -9999\n";
    file << "0.5 0.5 0.5\n0.5 0.5 0.5\n0.5 0.5 0.5\n";
  }

  void create_header_variant(const std::string &filename, double xll, double nodata,
                             bool mark_nodata, double yll = 0.0, double cellsize = 1.0) {
    std::ofstream file(filename);
    file << "ncols 3\nnrows 3\nxllcorner " << xll
         << "\nyllcorner " << yll << "\ncellsize " << cellsize << "\nNODATA_value "
         << nodata << "\n";
    file << (mark_nodata ? nodata : 0.5) << " 0.5 0.5\n0.5 0.5 0.5\n-9999 0.5 0.5\n";
  }
};

TEST_F(ErrorHandlingTest, MissingFiles) {
  auto node = YAML::Node();
  node["district_raster"] = "nonexistent.asc";
  EXPECT_THROW(spatial_data->process_config(node), std::runtime_error);
}

TEST_F(ErrorHandlingTest, RejectsMissingCellSizeAndRasterCatalog) {
  SpatialSettings settings;
  SpatialData missing_cell_size(&settings);
  YAML::Node without_cell_size;
  EXPECT_THROW(missing_cell_size.process_config(without_cell_size), std::runtime_error);

  SpatialData without_rasters(&settings);
  YAML::Node no_raster;
  no_raster["cell_size"] = 1.0;
  EXPECT_THROW(without_rasters.process_config(no_raster), std::runtime_error);
}

TEST_F(ErrorHandlingTest, RejectsMalformedAdministrativeBoundaryDefinitions) {
  SpatialSettings settings;
  SpatialData data(&settings);
  auto node = createBasicNode();
  node["administrative_boundaries"] = YAML::Load("{name: district}");
  EXPECT_THROW(data.process_config(node), std::runtime_error);

  node = createBasicNode();
  node["administrative_boundaries"] = YAML::Load("- name: district");
  EXPECT_THROW(data.process_config(node), std::runtime_error);
}

TEST_F(ErrorHandlingTest, MissingPopulationData) {
  auto node = createBasicNode();
  node.remove("population_raster");
  node.remove("population_size_by_location");
  SpatialSettings settings;
  SpatialData data(&settings);
  EXPECT_THROW(data.process_config(node), std::runtime_error);
}

TEST_F(ErrorHandlingTest, RejectsMissingRasterReferences) {
  SpatialSettings settings;
  SpatialData data(&settings);
  EXPECT_THROW(data.generate_locations(nullptr), std::runtime_error);
  AscFile reference;
  EXPECT_THROW(data.generate_locations(&reference), std::runtime_error);
}

TEST_F(ErrorHandlingTest, RejectsRasterWithMoreLocationsThanPopulation) {
  create_excess_value_raster("test_excess_beta.asc");
  auto node = createBasicNode();
  node["beta_raster"] = "test_excess_beta.asc";
  SpatialSettings settings;
  SpatialData data(&settings);
  EXPECT_THROW(data.process_config(node), std::runtime_error);
  std::remove("test_excess_beta.asc");
}

TEST_F(ErrorHandlingTest, InvalidDistrictNumbering) {
  create_invalid_district_raster("test_invalid.asc");
  create_population_raster("test_population.asc");
  auto node = createBasicNode();
  node["district_raster"] = "test_invalid.asc";
  EXPECT_THROW(spatial_data->process_config(node), std::runtime_error);
  std::remove("test_invalid.asc");
  std::remove("test_population.asc");
}

TEST_F(ErrorHandlingTest, MismatchedRasterDimensions) {
  create_district_raster("test_district.asc");
  create_mismatched_raster("test_mismatch.asc");
  auto node = createBasicNode();
  node["population_raster"] = "test_mismatch.asc";
  EXPECT_THROW(spatial_data->process_config(node), std::runtime_error);
  std::remove("test_district.asc");
  std::remove("test_mismatch.asc");
}

TEST_F(ErrorHandlingTest, InvalidLocationAccess) {
  EXPECT_THROW(
      { [[maybe_unused]] auto district = spatial_data->get_admin_unit("district", 999); },
      std::out_of_range);
}

TEST_F(ErrorHandlingTest, RejectsRasterHeaderAndNoDataMismatches) {
  create_header_variant("test_header_mismatch.asc", 1.0, -9999, false);
  auto node = createBasicNode();
  node["beta_raster"] = "test_header_mismatch.asc";
  SpatialSettings header_settings;
  SpatialData header_data(&header_settings);
  EXPECT_THROW(header_data.process_config(node), std::runtime_error);

  create_header_variant("test_nodata_mismatch.asc", 0.0, -8888, true);
  node = createBasicNode();
  node["beta_raster"] = "test_nodata_mismatch.asc";
  SpatialSettings nodata_settings;
  SpatialData nodata_data(&nodata_settings);
  EXPECT_THROW(nodata_data.process_config(node), std::runtime_error);
  std::remove("test_header_mismatch.asc");
  std::remove("test_nodata_mismatch.asc");

  create_header_variant("test_yll_mismatch.asc", 0.0, -9999, false, 1.0);
  node = createBasicNode();
  node["beta_raster"] = "test_yll_mismatch.asc";
  SpatialSettings yll_settings;
  SpatialData yll_data(&yll_settings);
  EXPECT_THROW(yll_data.process_config(node), std::runtime_error);

  create_header_variant("test_cellsize_mismatch.asc", 0.0, -9999, false, 0.0, 2.0);
  node = createBasicNode();
  node["beta_raster"] = "test_cellsize_mismatch.asc";
  SpatialSettings cellsize_settings;
  SpatialData cellsize_data(&cellsize_settings);
  EXPECT_THROW(cellsize_data.process_config(node), std::runtime_error);
  std::remove("test_yll_mismatch.asc");
  std::remove("test_cellsize_mismatch.asc");
}
