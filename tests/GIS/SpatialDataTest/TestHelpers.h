#ifndef SPATIAL_DATA_TEST_HELPERS_H
#define SPATIAL_DATA_TEST_HELPERS_H

#include <fstream>
#include <string>
#include <yaml-cpp/yaml.h>
#include "Configuration/Config.h"
#include "Simulation/Model.h"
#include "Spatial/GIS/SpatialData.h"

class SpatialDataTestHelper {
protected:
    void SetUp() {
        // Create test files
        create_district_raster("test_district.asc", false);  // Use 1-based districts
        create_population_raster("test_population.asc");
        
        // Initialize Model::CONFIG
        if (Model::get_config() == nullptr) {
            Model::get_instance().set_config(new Config());
        }

        Model::get_config()->get_spatial_settings().set_number_of_locations(0);

        // Initialize spatial data
        auto& spatial_data = SpatialData::get_instance();
        auto node = createBasicNode();
        spatial_data.parse(node);
    }

    YAML::Node createBasicNode() {
        YAML::Node node;
        node["district_raster"] = "test_district.asc";
        node["population_raster"] = "test_population.asc";
        node["cell_size"] = 1.0;
        
        // Create age distribution for all locations
        YAML::Node age_distributions;
        std::vector<double> dist = {0.2, 0.3, 0.5};
        for (int i = 0; i < 8; i++) {
            YAML::Node age_dist;
            for (const auto& val : dist) {
                age_dist.push_back(val);
            }
            age_distributions.push_back(age_dist);
        }
        node["age_distribution_by_location"] = age_distributions;
        
        // Add treatment probabilities for all locations
        YAML::Node treatment_under5;
        YAML::Node treatment_over5;
        for (int i = 0; i < 8; i++) {
            treatment_under5.push_back(0.5);
            treatment_over5.push_back(0.5);
        }
        node["p_treatment_for_less_than_5_by_location"] = treatment_under5;
        node["p_treatment_for_more_than_5_by_location"] = treatment_over5;
        
        // Add beta and population size for all locations
        YAML::Node betas;
        YAML::Node populations;
        for (int i = 0; i < 8; i++) {
            betas.push_back(0.5);
            populations.push_back(100);
        }
        node["beta_by_location"] = betas;
        node["population_size_by_location"] = populations;
        
        return node;
    }

    void TearDown() {
        cleanup_files();
        SpatialData::get_instance().reset();
        Model::get_config()->get_spatial_settings().set_number_of_locations(0);
        Model::get_instance().location_db().clear();
        
        if (Model::get_config() != nullptr) {
            delete Model::get_config();
            Model::get_instance().set_config(nullptr);
        }
    }

    static void create_district_raster(const std::string& filename, bool zero_based = true) {
        std::ofstream file(filename);
        // Basic ASC header
        file << "ncols 3\n";
        file << "nrows 3\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        // Data: 3x3 grid with districts 0,1 or 1,2 based on zero_based flag
        int base = zero_based ? 0 : 1;
        file << (base) << " " << (base) << " " << (base+1) << "\n";
        file << (base) << " " << (base+1) << " " << (base+1) << "\n";
        file << "-9999 " << (base+1) << " " << (base+1) << "\n";
        file.close();
    }

    static void create_population_raster(const std::string& filename) {
        std::ofstream file(filename);
        file << "ncols 3\n";
        file << "nrows 3\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        file << "100 150 200\n";
        file << "120 180 220\n";
        file << "-9999 160 240\n";
        file.close();
    }

    static void cleanup_files() {
        std::remove("test_district.asc");
        std::remove("test_population.asc");
        std::remove("test_invalid.asc");
    }
};

#endif 