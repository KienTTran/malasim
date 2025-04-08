#ifndef ADMIN_LEVEL_MANAGER_TEST_HELPERS_H
#define ADMIN_LEVEL_MANAGER_TEST_HELPERS_H

#include "Spatial/GIS/AdminLevelManager.h"
#include "Configuration/Config.h"
#include "Simulation/Model.h"
#include "Utils/Random.h"
#include "Population/Population.h"
#include "Spatial/Location/Location.h"
#include <fstream>
#include <memory>

#include "Spatial/GIS/AdminLevelManager.h"

class AdminLevelManagerTestHelper {
protected:
    AdminLevelManager manager;
    
    void SetUp() {
        // Set up any necessary test environment
        cleanup_files();
        
        if (Model::get_config() == nullptr) {
            Model::get_instance()->set_config(std::make_unique<Config>());
            
            // Set up minimal location database
            Model::get_config()->get_spatial_settings().set_number_of_locations(9);  // 3x3 grid
            auto& db = Model::get_config()->location_db();
            db.clear();
            
            // Create locations with proper initialization
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int loc_id = i * 3 + j;
                    float latitude = static_cast<float>(i);
                    float longitude = static_cast<float>(j);
                    // Initialize with id, lat, long, and a default population size of 1000
                    db.emplace_back(loc_id, latitude, longitude, 1000);
                }
            }
        }
    }
    
    void TearDown() {
        cleanup_files();
    }

    // Create a test raster file with specified dimensions and values
    static void create_test_raster(const std::string& filename, 
                                 bool zero_based = true,
                                 int rows = 3, 
                                 int cols = 3) {
        std::ofstream file(filename);
        file << "ncols " << cols << "\n";
        file << "nrows " << rows << "\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        
        // Write a simple pattern: each row has increasing values
        int start_value = zero_based ? 0 : 1;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (j > 0) file << " ";
                file << (start_value + (i * cols + j) % 3);  // Ensure only 3 unique values
            }
            file << "\n";
        }
        file.close();
    }

    // Create a test raster with specific values
    static void create_custom_raster(const std::string& filename, 
                                   const std::vector<std::vector<int>>& values) {
        std::ofstream file(filename);
        file << "ncols " << values[0].size() << "\n";
        file << "nrows " << values.size() << "\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        
        for (const auto& row : values) {
            for (size_t j = 0; j < row.size(); j++) {
                if (j > 0) file << " ";
                file << row[j];
            }
            file << "\n";
        }
        file.close();
    }

    static void cleanup_files() {
        std::remove("test_district.asc");
        std::remove("test_province.asc");
        std::remove("test_invalid.asc");
    }
};

#endif // ADMIN_LEVEL_MANAGER_TEST_HELPERS_H
