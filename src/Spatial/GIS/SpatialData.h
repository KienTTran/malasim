/*
 * SpatialData.h
 *
 * Definitions of the thread-safe singleton pattern spatial class which manages
 * the spatial aspects of the model from a high level.
 */
#ifndef SPATIALDATA_H
#define SPATIALDATA_H

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

#include "AscFile.h"

class SpatialData {
public:
  enum SpatialFileType {
    // Only use the data to define the model's location listing
    Locations,

    // Population data
    Population,

    // Transmission intensity, linked to the Entomological Inoculation Rates
    // (EIR)
    Beta,

    // District location
    Districts,

    // Travel time data
    Travel,

    // Eco-climatic zones that are used for seasonal variation
    Ecoclimatic,

    // Probability of treatment, under 5
    PrTreatmentUnder5,

    // Probability of treatment, over 5
    PrTreatmentOver5,

    // Number of sequential items in the type
    Count
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
    bool matches(const RasterInformation &other) const {
      return number_columns == other.number_columns
             && number_rows == other.number_rows
             && x_lower_left_corner == other.x_lower_left_corner
             && y_lower_left_corner == other.y_lower_left_corner
             && cellsize == other.cellsize
             && no_data_value == other.no_data_value;
    }

    // Check if the raster information has been initialized
    bool is_initialized() const {
      return number_columns != NOT_SET && number_rows != NOT_SET
             && x_lower_left_corner != NOT_SET && y_lower_left_corner != NOT_SET
             && cellsize != NOT_SET
             && no_data_value != NOT_SET;
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

  const std::string BETA_RASTER = "beta_raster";
  const std::string DISTRICT_RASTER = "district_raster";
  const std::string LOCATION_RASTER = "location_raster";
  const std::string POPULATION_RASTER = "population_raster";
  const std::string TRAVEL_RASTER = "travel_raster";
  const std::string ECOCLIMATIC_RASTER = "ecoclimatic_raster";
  const std::string TREATMENT_RATE_UNDER5 = "p_treatment_under_5_raster";
  const std::string TREATMENT_RATE_OVER5 = "p_treatment_over_5_raster";

  // The size of the cells in the raster, the units shouldn't matter, but this
  // was written when we were using 5x5 km cells
  float cell_size = 0;

  // First district index, default -1, lazy initialization to actual value
  int first_district = -1;

  // Count of district loaded in the map, default zero, lazy initialization to
  // actual value
  int district_count = 0;

  int min_district_id = -1;
  int max_district_id = -1;

  // Add raster_info as a data member
  RasterInformation raster_info;

  // true if any raster file has been loaded, false otherwise
  bool using_raster = false;

  // Constructor
  SpatialData();

  // Deconstructor
  ~SpatialData();

public:
  /**
   * @brief Parses spatial configuration from YAML and initializes the spatial
   * system, all reaster files must be defined in raster_db node for the check_catalog to work
   *
   * @param node YAML configuration node containing spatial settings
   * @return true if parsing was successful
   * @throws std::runtime_error if required configuration is missing or invalid
   */
  bool parse(const YAML::Node &node);

  // Check the loaded spatial catalog for errors, returns true if there are
  // errors
  bool check_catalog(std::string &errors);

  /**
   * @brief Generates location database from a reference raster file
   * @param raster The raster file to use as a reference for location generation
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
  void generate_locations(AscFile* raster);

  // Load the given raster file into the spatial catalog and assign the given
  // label
  void load(const std::string &filename, SpatialFileType type);

  // Load all the spatial data from the node
  void load_files(const YAML::Node &node);

  // copy the raster to the location_db; works with betas and probability of
  // treatment
  void copy_raster_to_location_db(SpatialFileType type);

  // Perform any clean-up operations after parsing the YAML file is complete
  void parse_complete();

  // Not supported by singleton.
  SpatialData(SpatialData const &) = delete;

  // Not supported by singleton.
  void operator=(SpatialData const &) = delete;

  // Get a reference to the spatial object.
  static SpatialData &get_instance() {
    static SpatialData instance;
    return instance;
  }

  // Return the raster header or the default structure if no raster are loaded
  RasterInformation get_raster_header();

  // Generate the Euclidean distances for the location_db
  void generate_distances() const;

  /**
   * @brief Retrieves the district ID as defined in raster files, corresponding
   * to a given location ID.
   *
   * This function maps a location ID to its corresponding district ID, as
   * defined within the raster files used by the simulation. This mapping is
   * crucial for spatial analyses and operations that require understanding the
   * geographical district a specific location belongs to.
   *
   * @param location The location ID for which the district ID is requested.
   * @return The district ID matching the location ID, as defined in the raster
   * files.
   */
  int get_raster_district(int location);

  /**
   * @brief Retrieves the 0-based district ID in the simulation, corresponding
   * to a given location ID.
   *
   * This function translates a location ID into a simulation-internal, 0-based
   * district ID. It is used primarily for internal simulation operations where
   * districts are handled with 0-based indexing.
   *
   * @param location The location ID for which the 0-based district ID is
   * requested.
   * @return The 0-based district ID corresponding to the given location ID.
   */
  int get_district(int location);

  /**
   * @brief Returns a vector of location IDs that belong to the specified district.
   * Uses a pre-computed mapping for efficient lookups.
   *
   * @param district The district ID (matches IDs from the raster file, can be 0-based or 1-based)
   * @return const reference to vector of location IDs in the district
   * @throws std::runtime_error if districts are not loaded
   * @throws std::out_of_range if district ID is invalid
   */
  const std::vector<int>& get_district_locations(int district) const;

  /**
   * @brief Returns the adjusted district index matching the definition in
   * raster files, for external storage.
   *
   * This function adjusts an internal simulation district index to match the
   * district indexing as defined in the raster files. The adjusted index is
   * suitable for external references, such as database entries, ensuring
   * consistency between the simulation's internal data representation and
   * external data stores.
   *
   * @param simulationDistrict The internal simulation district index that needs
   * adjustment for external use.
   * @return The adjusted district index that matches the raster file
   * definitions, suitable for external references.
   */
  int adjust_simulation_district_to_raster_index(int simulationDistrict) {
    // Assuming get_first_district() returns the necessary adjustment. Adjust as
    // per your actual logic.
    return simulationDistrict + get_first_district();
  }

  // Get the locations that are within the given district, throws an error if
  // not districts are loaded
  std::vector<int> get_district_locations(int district);

  // Returns the index of the first district.
  // Note that the index may be one (ArcGIS default) or zero; however, a
  // delayed error is generated if the value is not one of the two.
  int get_first_district();

  // Get a reference to the AscFile raster, may be a nullptr
  AscFile* get_raster(SpatialFileType type) { return data_[type].get(); }

  /**
   * @brief Populates dependent data structures after input data and raster
   * files have been processed.
   *
   * This function is designed to run once all necessary input data and raster
   * files have been read and processed. It performs the crucial step of
   * populating several dependent data structures that are essential for the
   * simulation's operation. These include caching the total count of districts,
   * determining the first district index based on the spatial data, and
   * creating the district lookup table. The function ensures that these
   * components are correctly initialized before the simulation proceeds,
   * guaranteeing that spatial queries and operations can be conducted
   * efficiently.
   *
   * @note This function should be called after all input and raster data have
   * been fully processed but before the simulation begins to ensure that all
   * dependent data structures are accurately populated. Failure to call this
   * function in the correct sequence may result in uninitialized or incorrect
   * data being used in the simulation, leading to potential errors or
   * inaccurate results.
   *
   * @pre Raster files and input data must be loaded and processed. This
   * includes loading spatial data such as district boundaries and population
   * distributions from raster files.
   *
   * @post The total district count is cached, the first district index is
   * determined, and the district lookup table is created and populated. These
   * actions prepare the system for efficient spatial operations and
   * simulations.
   */
  void populate_dependent_data();

  // Reset the singleton instance for testing
  void reset() {
    // Reset each unique_ptr individually
    for (auto &ptr : data_) { ptr.reset(); }
    district_count = -1;
    min_district_id = -1;
    max_district_id = -1;
  }

  // Add method to validate raster information
  bool validate_raster_info(const RasterInformation &new_info,
                            std::string &errors);

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


    /**
   * @brief Retrieves the district ID from the district raster file for a given location.
   *
   * This is an internal method used during initialization to build the district lookup table.
   * It performs coordinate-based lookups in the district raster file.
   *
   * @param location The location ID for which the district ID is requested.
   * @return The district ID from the raster file for the given location.
   * @throws std::out_of_range if location or coordinates are invalid
   * @throws std::runtime_error if district data is not loaded or coordinates are null
   */
  int get_district_from_raster(int location);

private:
  /**
  * @brief This property holds a pre-populated map from location to district ID
  * as defined in the GIS raster file.
  *
  * The vector contains district IDs where each element represents a specific
  * location, and the value at each index corresponds to the actual district ID
  * from the raster file. The district IDs can be either 0-based or 1-based,
  * depending on how they are defined in the input GIS raster file. This mapping
  * is essential for quickly determining the district of any given location
  * within the simulation. It is assumed that the mapping is set up during the
  * initialization phase (in SpatialData::parse()) and remains constant
  * throughout the simulation, facilitating efficient spatial queries and
  * analyses.
  */
  std::vector<int> location_to_district_;

  // Maps district IDs to their corresponding location IDs
  // Index is 0 to max_district_id+1 to handle both 0-based and 1-based IDs
  std::vector<std::vector<int>> district_to_locations_;

  // Initialize array with nullptr
  std::array<std::unique_ptr<AscFile>, SpatialFileType::Count> data_{};
};

#endif
