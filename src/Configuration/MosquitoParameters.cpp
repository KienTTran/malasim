#include "MosquitoParameters.h"

#include "Simulation/Model.h"

// the setting mode here is used to assign ifr and size to the population regardless
// the population was created using grid based or location based.
void MosquitoParameters::process_config_using_locations(std::vector<Spatial::Location> &locations) {
  spdlog::info("Processing MosquitoParameters");
  if (get_mosquito_config().get_mode() == SpatialSettings::GRID_BASED_MODE) {
    spdlog::info("Processing MosquitoParameters using grid based mode");
    AscFile* size_raster =
        Model::get_spatial_data()->get_raster(SpatialData::SpatialFileType::MOSQUITO_SIZE);
    if (size_raster == nullptr) {
      throw std::invalid_argument("Mosquito raster flag set without mosquito size raster loaded.");
    }
    // Prepare to run
    spdlog::info("Setting mosquito size using raster data.");
    // Load the values based upon the raster data
    int index = 0;
    for (int row = 0; row < size_raster->nrows; row++) {
      for (int col = 0; col < size_raster->ncols; col++) {
        // Pass if we have no data here
        if (size_raster->data[row][col] == size_raster->nodata_value) { continue; }
        // Set the seasonal period
        locations[index].mosquito_size = size_raster->data[row][col];
        index++;
      }
    }
    AscFile* ifr_raster =
        Model::get_spatial_data()->get_raster(SpatialData::SpatialFileType::MOSQUITO_IFR);
    if (ifr_raster == nullptr) {
      throw std::invalid_argument("Mosquito raster flag set without mosquito ifr raster loaded.");
    }
    spdlog::info("Setting mosquito ifr using raster data.");
    // Load the values based upon the raster data
    index = 0;
    for (int row = 0; row < ifr_raster->nrows; row++) {
      for (int col = 0; col < ifr_raster->ncols; col++) {
        // Pass if we have no data here
        if (ifr_raster->data[row][col] == ifr_raster->nodata_value) { continue; }
        // Set the seasonal period
        locations[index].mosquito_ifr = ifr_raster->data[row][col];
        index++;
      }
    }
  }
  if (get_mosquito_config().get_mode() == SpatialSettings::LOCATION_BASED_MODE) {
    spdlog::info("Processing MosquitoParameters using location based mode");
    LocationBased location_based = get_mosquito_config().get_location_based();
    if (location_based.get_interrupted_feeding_rate().size() == 1) {
      spdlog::info("1 IFR value provided, distributing equally to all locations");
      for (auto &location : locations) {
        location.mosquito_ifr = location_based.get_interrupted_feeding_rate()[0];
        location.mosquito_size = location_based.get_prmc_size()[0];
      }
    }
    if (location_based.get_interrupted_feeding_rate().size() == locations.size()) {
      spdlog::info("IFR values provided for all locations");
      for (auto i = 0; i < locations.size(); i++) {
        locations[i].mosquito_ifr = location_based.get_interrupted_feeding_rate()[i];
        locations[i].mosquito_size = location_based.get_prmc_size()[i];
      }
    }
  }
}

