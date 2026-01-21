#ifndef TEST_FILE_GENERATORS_H
#define TEST_FILE_GENERATORS_H

#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace test_fixtures {

/**
 * @brief Generate a minimal valid ASC raster file for testing
 */
inline void create_test_raster_file(
    const std::string& filename,
    int ncols = 10,
    int nrows = 10,
    double default_value = 100.0,
    double nodata_value = -9999.0) {
  
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to create test raster file: " + filename);
  }

  file << "ncols " << ncols << "\n";
  file << "nrows " << nrows << "\n";
  file << "xllcorner 0.0\n";
  file << "yllcorner 0.0\n";
  file << "cellsize 5000\n";
  file << "NODATA_value " << nodata_value << "\n";

  for (int row = 0; row < nrows; ++row) {
    for (int col = 0; col < ncols; ++col) {
      if ((row < 2 && col < 2) || (row >= nrows - 2 && col >= ncols - 2)) {
        file << default_value;
      } else {
        file << nodata_value;
      }
      if (col < ncols - 1) file << " ";
    }
    file << "\n";
  }

  file.close();
}

/**
 * @brief Generate a district raster with multiple district IDs
 * Creates 3 districts (IDs 1, 2, 3) for proper testing
 */
inline void create_test_district_raster(
    const std::string& filename,
    int ncols = 10,
    int nrows = 10,
    double nodata_value = -9999.0) {
  
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to create test district raster: " + filename);
  }

  file << "ncols " << ncols << "\n";
  file << "nrows " << nrows << "\n";
  file << "xllcorner 0.0\n";
  file << "yllcorner 0.0\n";
  file << "cellsize 5000\n";
  file << "NODATA_value " << nodata_value << "\n";

  // Create 3 districts in different areas
  // District 1: top-left corner
  // District 2: bottom-left corner  
  // District 3: bottom-right corner
  for (int row = 0; row < nrows; ++row) {
    for (int col = 0; col < ncols; ++col) {
      if (row < 2 && col < 2) {
        file << "1";  // District 1
      } else if (row >= nrows - 2 && col < 2) {
        file << "2";  // District 2
      } else if (row >= nrows - 2 && col >= ncols - 2) {
        file << "3";  // District 3
      } else {
        file << nodata_value;
      }
      if (col < ncols - 1) file << " ";
    }
    file << "\n";
  }

  file.close();
}

/**
 * @brief Generate a raster with only 2 locations (for tests that assume 2 locations)
 * Creates 10x10 raster with data in only 2 cells
 */
inline void create_test_raster_2_locations(
    const std::string& filename,
    double default_value = 100.0,
    double nodata_value = -9999.0) {
  
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to create test raster file: " + filename);
  }

  file << "ncols 10\n";
  file << "nrows 10\n";
  file << "xllcorner 0.0\n";
  file << "yllcorner 0.0\n";
  file << "cellsize 5000\n";
  file << "NODATA_value " << nodata_value << "\n";

  // Create 10x10 raster with only 2 data cells (locations 0 and 1)
  for (int row = 0; row < 10; ++row) {
    for (int col = 0; col < 10; ++col) {
      if (row == 1 && col == 2) {
        file << default_value;  // Location 0
      } else if (row == 4 && col == 5) {
        file << default_value;  // Location 1
      } else {
        file << nodata_value;
      }
      if (col < 9) file << " ";
    }
    file << "\n";
  }

  file.close();
}

/**
 * @brief Generate seasonality CSV file
 */
inline void create_test_seasonality_file(const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to create seasonality file: " + filename);
  }

  file << "location,month,a\n";
  for (int loc = 0; loc < 2; ++loc) {
    for (int month = 1; month <= 12; ++month) {
      double a_value = (month >= 6 && month <= 9) ? 1.2 : 0.8;
      file << loc << "," << month << "," << a_value << "\n";
    }
  }
  file.close();
}

/**
 * @brief Create all raster files referenced in test YAML
 */
inline void create_raster_files_from_yaml(const YAML::Node& config) {
  // Grid-based rasters
  if (config["spatial_settings"]["grid_based"]) {
    auto grid = config["spatial_settings"]["grid_based"];
    
    if (grid["population_raster"]) {
      create_test_raster_file(grid["population_raster"].as<std::string>(), 10, 10, 1000.0);
    }
    if (grid["beta_raster"]) {
      create_test_raster_file(grid["beta_raster"].as<std::string>(), 10, 10, 0.5);
    }
    if (grid["p_treatment_under_5_raster"]) {
      create_test_raster_file(grid["p_treatment_under_5_raster"].as<std::string>(), 10, 10, 0.6);
    }
    if (grid["p_treatment_over_5_raster"]) {
      create_test_raster_file(grid["p_treatment_over_5_raster"].as<std::string>(), 10, 10, 0.6);
    }
    if (grid["ecoclimatic_raster"]) {
      create_test_raster_file(grid["ecoclimatic_raster"].as<std::string>(), 10, 10, 1.0);
    }
    if (grid["travel_raster"]) {
      create_test_raster_file(grid["travel_raster"].as<std::string>(), 10, 10, 0.1);
    }
    
    // Administrative boundaries - use special district raster generator
    if (grid["administrative_boundaries"]) {
      for (const auto& boundary : grid["administrative_boundaries"]) {
        if (boundary["raster"]) {
          std::string raster_path = boundary["raster"].as<std::string>();
          // Check if it's a district raster and use special generator
          if (raster_path.find("district") != std::string::npos) {
            create_test_district_raster(raster_path, 10, 10);
          } else {
            create_test_raster_file(raster_path, 10, 10, 1.0);
          }
        }
      }
    }
  }
  
  // Mosquito rasters
  if (config["mosquito_parameters"]["grid_based"]) {
    auto mosquito = config["mosquito_parameters"]["grid_based"];
    if (mosquito["interrupted_feeding_rate_raster"]) {
      create_test_raster_file(mosquito["interrupted_feeding_rate_raster"].as<std::string>(), 10, 10, 0.1);
    }
    if (mosquito["prmc_size_raster"]) {
      create_test_raster_file(mosquito["prmc_size_raster"].as<std::string>(), 10, 10, 100000.0);
    }
  }
  
  // Seasonality files
  if (config["seasonality_settings"]["pattern"]["filename"]) {
    create_test_seasonality_file(config["seasonality_settings"]["pattern"]["filename"].as<std::string>());
  }
  if (config["seasonality_settings"]["rainfall"]["filename"]) {
    std::string filename = config["seasonality_settings"]["rainfall"]["filename"].as<std::string>();
    // Don't duplicate if same file
    if (!config["seasonality_settings"]["pattern"]["filename"] || 
        filename != config["seasonality_settings"]["pattern"]["filename"].as<std::string>()) {
      create_test_seasonality_file(filename);
    }
  }
}

/**
 * @brief Clean up all generated test files
 * Removes all test_* files to ensure clean state
 */
inline void cleanup_test_files() {
  // Remove common test files explicitly
  std::vector<std::string> files = {
    "test_input.yml",
    "test_init_pop.asc",
    "test_district.asc",
    "test_treatment.asc",
    "test_beta.asc",
    "test_beta_r1.asc",
    "test_ecozone.asc",
    "test_travel.asc",
    "test_mosquito_ifr.asc",
    "test_mosquito_size.asc",
    "test_seasonality.csv",
    "test_seasonality_pattern.csv",
    "test_pop.asc",
    "test_population.asc"
  };

  for (const auto& file : files) {
    std::filesystem::remove(file);
  }
  
  // Also remove any .db files
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(".", ec)) {
    if (entry.is_regular_file()) {
      std::string filename = entry.path().filename().string();
      if (filename.ends_with(".db")) {
        std::filesystem::remove(entry.path(), ec);
      }
    }
  }
}

/**
 * @brief Setup test environment from template
 * @param output_filename Output YAML filename (default: "test_input.yml")
 * @param modify_callback Optional callback to modify config before writing
 * @return The loaded YAML config
 */
inline YAML::Node setup_test_environment(
    const std::string& output_filename = "test_input.yml",
    std::function<void(YAML::Node&)> modify_callback = nullptr) {
  
  // Clean up any leftover test files from previous runs
  cleanup_test_files();
  
  // Find and load template
  std::vector<std::string> template_paths = {
    "test_input_template.yml",
    "fixtures/test_input_template.yml",
    "../tests/fixtures/test_input_template.yml",
    "../../tests/fixtures/test_input_template.yml"
  };
  
  YAML::Node config;
  bool loaded = false;
  
  for (const auto& path : template_paths) {
    try {
      config = YAML::LoadFile(path);
      loaded = true;
      break;
    } catch (...) {
      continue;
    }
  }
  
  if (!loaded) {
    throw std::runtime_error("Failed to load test_input_template.yml from any expected location");
  }
  
  // Allow caller to modify config
  if (modify_callback) {
    modify_callback(config);
  }
  
  // Write modified config
  std::ofstream out(output_filename);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to create output file: " + output_filename);
  }
  out << config;
  out.close();
  
  // Create all raster files referenced in the config
  create_raster_files_from_yaml(config);
  
  return config;
}

/**
 * @brief Simple setup without modifications
 */
inline void create_complete_test_environment() {
  setup_test_environment("test_input.yml");
}

} // namespace test_fixtures

#endif // TEST_FILE_GENERATORS_H
