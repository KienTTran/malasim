/*
 * SpatialData.h
 *
 * Definitions of the thread-safe singleton pattern spatial class which manages
 * the spatial aspects of the model from a high level.
 */
#ifndef SPATIALDATA_H
#define SPATIALDATA_H

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>

#include "AdminLevelManager.h"
#include "AscFile.h"

class SpatialSettings;

class SpatialData {
public:
  enum SpatialFileType : uint8_t {
    // Only use the data to define the model's location listing
    LOCATIONS = 0,

    // Population data
    POPULATION,

    // Transmission intensity, linked to the Entomological Inoculation Rates
    // (EIR)
    BETA,

    // District location
    DISTRICTS,

    // Travel time data
    TRAVEL,

    // Eco-climatic zones that are used for seasonal variation
    ECOCLIMATIC,

    // Probability of treatment, under 5
    PR_TREATMENT_UNDER5,

    // Probability of treatment, over 5
    PR_TREATMENT_OVER5,

    // Mosquito size
    MOSQUITO_SIZE,

    // Interrupted Feeding Rate
    MOSQUITO_IFR,

    // Number of sequential items in the type
    COUNT
  };

  struct RasterInformation {
    // Flag to indicate the value has not been set yet
    static const int NOT_SET = -1;

    // The number of columns in the raster
    int number_columns = NOT_SET;

    // The number of rows in the raster
    int number_rows = NOT_SET;

    // The lower-left X coordinate of the raster
    double x_lower_left_corner = NOT_SET;

    // The lower-left Y coordinate of the raster
    double y_lower_left_corner = NOT_SET;

    // The size of the cell, typically in meters
    double cellsize = NOT_SET;

    double no_data_value = NOT_SET;

    // Validate if the raster information matches another instance
    [[nodiscard]] bool matches(const RasterInformation &other) const {
      return number_columns == other.number_columns && number_rows == other.number_rows
             && x_lower_left_corner == other.x_lower_left_corner
             && y_lower_left_corner == other.y_lower_left_corner && cellsize == other.cellsize
             && no_data_value == other.no_data_value;
    }

    // Check if the raster information has been initialized
    [[nodiscard]] bool is_initialized() const {
      return number_columns != NOT_SET && number_rows != NOT_SET && x_lower_left_corner != NOT_SET
             && y_lower_left_corner != NOT_SET && cellsize != NOT_SET && no_data_value != NOT_SET;
    }

    /* For testing only */
    void reset() {
      number_columns = NOT_SET;
      number_rows = NOT_SET;
      x_lower_left_corner = NOT_SET;
      y_lower_left_corner = NOT_SET;
      cellsize = NOT_SET;
      no_data_value = NOT_SET;
    }
  };

  /**
   * @brief This property holds a pre-populated map from location to district
   * using a 0-based index.
   *
   * The vector contains indices where each element represents a specific
   * location, and the value at each index corresponds to the district that
   * location belongs to. This mapping is essential for quickly determining the
   * district of any given location within the simulation. It is assumed that
   * the mapping is set up during the initialization phase (in
   * SpatialData::parse()) and remains constant throughout the
   * simulation, facilitating efficient spatial queries and analyses.
   */

  constexpr static const std::string_view BETA_RASTER = "beta_raster";
  constexpr static const std::string_view DISTRICT_RASTER = "district_raster";
  constexpr static const std::string_view LOCATION_RASTER = "location_raster";
  constexpr static const std::string_view POPULATION_RASTER = "population_raster";
  constexpr static const std::string_view TRAVEL_RASTER = "travel_raster";
  constexpr static const std::string_view ECOCLIMATIC_RASTER = "ecoclimatic_raster";
  constexpr static const std::string_view TREATMENT_RATE_UNDER5 = "p_treatment_under_5_raster";
  constexpr static const std::string_view TREATMENT_RATE_OVER5 = "p_treatment_over_5_raster";
  constexpr static const std::string_view MOSQUITO_SIZE_RASTER = "prmc_size_raster";
  constexpr static const std::string_view MOSQUITO_IFR_RASTER = "interrupted_feeding_rate_raster";

  // Add constant for the new admin boundaries configuration section
  constexpr static const std::string_view ADMIN_BOUNDARIES = "administrative_boundaries";

  // Disallow copy
  SpatialData(const SpatialData&) = delete;
  SpatialData& operator=(const SpatialData&) = delete;

  // Disallow move
  SpatialData(SpatialData&&) = delete;
  SpatialData& operator=(SpatialData&&) = delete;

  explicit SpatialData(SpatialSettings* spatial_settings);
  // Deconstructor
  ~SpatialData();

  /**
   * @brief Parses spatial configuration from YAML and initializes the spatial
   * system, all reaster files must be defined in raster_db node for the check_catalog to work
   *
   * @param node YAML configuration node containing spatial settings
   * @return true if parsing was successful
   * @throws std::runtime_error if required configuration is missing or invalid
   */
  bool process_config(const YAML::Node &node);

  // Check the loaded spatial catalog for errors, returns true if there are
  // errors
  bool check_catalog(std::string &errors);

  /**
   * @brief Generates location database from a reference raster file
   * @param reference The raster file to use as a reference for location generation
   *
   * This function creates location entries for each valid (non-NODATA) cell in
   * the raster. Each location is assigned:
   * - A unique sequential ID
   * - Row and column coordinates from the raster
   * - Initial elevation of 0
   *
   * @throws std::runtime_error if no valid raster files are available
   * @throws std::runtime_error if no valid locations are found in the raster
   */
  void generate_locations(AscFile* reference);

  // Load the given raster file into the spatial catalog and assign the given
  // label
  void load(const std::string &filename, SpatialFileType type);

  // Load all the spatial data from the node
  void load_files(const YAML::Node &node);

  // copy the raster to the location_db; works with betas and probability of
  // treatment
  void populate_raster_data_to_location_db(SpatialFileType type);

  // Perform any clean-up operations after parsing the YAML file is complete
  void parse_complete();

  // Return the raster header or the default structure if no raster are loaded
  [[nodiscard]] RasterInformation get_raster_header() const;

  // Generate the Euclidean distances for the location_db
  void generate_distances() const;

  [[nodiscard]] AdminLevelManager* get_admin_level_manager() const { return admin_manager_.get(); }

  /**
   * @brief Retrieves the administrative unit ID for a given location
   * @param location The location ID
   * @param level_name The administrative level name (e.g., "district")
   * @return The administrative unit ID for the given location
   * @throws std::out_of_range if location is invalid
   * @throws std::runtime_error if admin level is not initialized
   */
  [[nodiscard]] int get_admin_unit(const std::string &level_name, int location) const {
    return admin_manager_->get_admin_unit(level_name, location);
  }

  /**
   * @brief Retrieves the administrative unit ID for a given location
   * @param level_id The administrative level ID
   * @param location The location ID
   * @return The administrative unit ID for the given location
   * @throws std::out_of_range if location is invalid
   * @throws std::runtime_error if admin level is not initialized
   */
  [[nodiscard]] int get_admin_unit(int level_id, int location) const {
    return admin_manager_->get_admin_unit(level_id, location);
  }

  [[nodiscard]] int get_admin_level_id(const std::string &level_name) const {
    return admin_manager_->get_admin_level_id(level_name);
  }

  [[nodiscard]] const std::string &get_admin_level_name(int level_id) const {
    return admin_manager_->get_level_names()[level_id];
  }

  /**
   * @brief Returns locations in the specified administrative unit
   * @param unit_id The administrative unit ID
   * @param level_name The administrative level name (e.g., "district")
   * @return Const reference to vector of location IDs in the administrative unit
   * @throws std::runtime_error if admin level is not initialized
   */
  [[nodiscard]] const std::vector<int> &get_locations_in_unit(const std::string &level_name,
                                                              int unit_id) const {
    return admin_manager_->get_locations_in_unit(level_name, unit_id);
  }

  [[nodiscard]] const std::vector<int> &get_locations_in_unit(int level_id, int unit_id) const {
    return admin_manager_->get_locations_in_unit(level_id, unit_id);
  }

  /**
   * @brief Returns the number of units in the specified administrative level
   * @param level_id The administrative level ID
   * @return The number of units in the administrative level
   * @throws std::runtime_error if admin level is not initialized
   */
  [[nodiscard]] int get_unit_count(int level_id) const {
    return admin_manager_->get_unit_count(level_id);
  }

  /**
   * @brief Returns the number of units in the specified administrative level
   * @param level_name The administrative level name (e.g., "district")
   * @return The number of units in the administrative level
   * @throws std::runtime_error if admin level is not initialized
   */
  [[nodiscard]] int get_unit_count(const std::string &level_name) const {
    if (admin_manager_ == nullptr) { return -1; }
    return admin_manager_->get_unit_count(level_name);
  }

  [[nodiscard]] const BoundaryData* get_boundary(const std::string &level_name) const {
    if (admin_manager_ == nullptr) { return nullptr; }
    return admin_manager_->get_boundary(level_name);
  }

  /**
   * @brief Initializes and configures administrative boundaries for the simulation.
   *
   * This function is designed to run once all necessary input data and raster
   * files have been read and processed. It sets up the AdminLevelManager and
   * registers any administrative levels (like districts) found in the raster data.
   * The administrative boundaries are essential for spatial queries and operations
   * in the simulation, allowing locations to be grouped by administrative units.
   *
   * @note This function should be called after all input and raster data have
   * been fully processed but before the simulation begins to ensure that all
   * administrative boundaries are accurately configured.
   *
   * @pre Raster files and input data must be loaded and processed, including
   * any administrative boundary rasters (like district boundaries).
   *
   * @post The AdminLevelManager is initialized with all relevant administrative levels,
   * and administrative boundary data is configured for use in spatial operations.
   */
  void initialize_admin_boundaries();

  // Get a reference to the AscFile raster, may be a nullptr
  AscFile* get_raster(SpatialFileType type) { return data_.at(type).get(); }

  // Add method to validate raster information
  bool validate_raster_info(const RasterInformation &new_info, std::string &errors);

  /**
   * @brief Returns a list of all available administrative levels
   * @return Vector of administrative level names
   */
  [[nodiscard]] const std::vector<std::string> &get_admin_levels() const {
    if (admin_manager_ == nullptr) {
      static const std::vector<std::string> EMPTY_VECTOR;
      return EMPTY_VECTOR;
    }
    return admin_manager_->get_level_names();
  }

  /**
   * @brief Checks if an administrative level exists
   * @param level_name The administrative level name to check
   * @return true if the level exists, false otherwise
   */
  [[nodiscard]] bool has_admin_level(const std::string &level_name) const {
    if (admin_manager_ == nullptr) { return false; }
    return admin_manager_->has_level(level_name);
  }

  /**
   * @brief Gets all units in an administrative level
   * @param level_name The administrative level name
   * @return Pair of min and max unit IDs for the requested level
   * @throws std::runtime_error if admin level does not exist
   */
  [[nodiscard]] std::pair<int, int> get_admin_units(const std::string &level_name) const {
    if (admin_manager_ == nullptr) {
      // return an invalid pair
      return {-1, -1};
    }
    return admin_manager_->get_units(level_name);
  }

  /**
   * @brief Loads age distribution data from YAML configuration
   * @throws std::runtime_error if age distribution data is invalid
   */
  void load_age_distribution(const YAML::Node &node);

  /**
   * @brief Loads treatment data from YAML configuration if not provided by
   * raster
   * @throws std::runtime_error if treatment data is invalid
   */
  void load_treatment_data(const YAML::Node &node);

  /**
   * @brief Loads beta and population data from YAML if not provided by raster
   * @throws std::runtime_error if required data is missing or invalid
   */
  void load_location_data(const YAML::Node &node);

  /*
   * Reset the raster information, clearing all raster data.
   */
  void reset_raster_info() {
    spdlog::warn("Reset raster info. All raster data will be lost.");
    raster_info_.reset();
  }

  [[nodiscard]] float get_cell_size() const { return cell_size_; }
  void set_cell_size(float cell_size) { cell_size_ = cell_size; }

  [[nodiscard]] bool get_using_raster() const { return using_raster_; }
  void set_using_raster(bool using_raster) { using_raster_ = using_raster; }

  [[nodiscard]] const RasterInformation &get_raster_info() const { return raster_info_; }
  void set_raster_info(const RasterInformation &raster_info) { raster_info_ = raster_info; }

private:
  SpatialSettings* spatial_settings_;
  std::array<std::unique_ptr<AscFile>, SpatialFileType::COUNT> data_{};

  // Map of admin level names to their corresponding raster paths
  std::map<std::string, std::string> admin_rasters_;

  // Add AdminLevelManager as a member
  std::unique_ptr<AdminLevelManager> admin_manager_{std::make_unique<AdminLevelManager>()};

  // Helper method to parse administrative boundaries from YAML
  void load_admin_boundaries(const YAML::Node &node);
  // The size of the cells in the raster, the units shouldn't matter, but this
  // was written when we were using 5x5 km cells
  float cell_size_{0};

  // Add raster_info as a data member
  RasterInformation raster_info_;

  // true if any raster file has been loaded, false otherwise
  bool using_raster_{false};
};

#endif
