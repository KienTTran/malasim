/*
 * SpatialData.cpp
 *
 * Implementation of SpatialData functions.
 */
#include "SpatialData.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

#include "Configuration/SpatialSettings/SpatialSettings.h"
#include "Utils/Helpers/StringHelpers.h"

SpatialData::SpatialData(SpatialSettings* spatial_settings) : spatial_settings_(spatial_settings) {}

SpatialData::~SpatialData() = default;  // Let unique_ptr handle cleanup

bool SpatialData::process_config(const YAML::Node &node) {
  // Validate required configuration
  if (!node["cell_size"]) {
    throw std::runtime_error("Missing required 'cell_size' configuration");
  }
  cell_size_ = node["cell_size"].as<float>();

  // Load and validate raster files
  load_files(node);

  // number of lcaitions is in Spatial Settings
  if (spatial_settings_->get_number_of_locations() != 0) {
    throw std::runtime_error("Location database is not empty");
  }

  // find first raster that is not nullptr
  auto* first_raster =
      std::ranges::find_if(data_, [](const auto &raster) { return raster != nullptr; });
  if (first_raster == data_.end()) { throw std::runtime_error("No raster files found"); }

  spdlog::info("Location database is empty, generating locations using first available raster.");
  generate_locations(first_raster->get());
  generate_distances();

  // Load the age distribution data from the YAML file (not provided by raster)
  load_age_distribution(node);

  // Load location-specific data from raster or YAML
  load_location_data(node);
  load_treatment_data(node);

  // Finalize setup, populate location_to_district, district_to_locations
  initialize_admin_boundaries();

  // Reset rasters
  parse_complete();
  return true;
}

bool SpatialData::validate_raster_info(const RasterInformation &new_info, std::string &errors) {
  // If raster_info isn't initialized yet, store the new info
  if (!raster_info_.is_initialized()) {
    raster_info_ = new_info;
    return true;
  }

  // Otherwise validate that the new info matches
  if (!raster_info_.matches(new_info)) {
    // Use a vector to store error messages
    std::vector<std::string> error_messages;

    if (new_info.number_columns != raster_info_.number_columns) {
      spdlog::info("{} vs {}", new_info.number_columns, raster_info_.number_columns);
      error_messages.emplace_back("mismatched number of columns");
    }
    if (new_info.number_rows != raster_info_.number_rows) {
      spdlog::info("{} vs {}", new_info.number_rows, raster_info_.number_rows);
      error_messages.emplace_back("mismatched number of rows");
    }
    if (new_info.x_lower_left_corner != raster_info_.x_lower_left_corner) {
      error_messages.emplace_back("mismatched x lower left corner");
    }
    if (new_info.y_lower_left_corner != raster_info_.y_lower_left_corner) {
      error_messages.emplace_back("mismatched y lower left corner");
    }
    if (new_info.cellsize != raster_info_.cellsize) {
      error_messages.emplace_back("mismatched cell size");
    }

    // Join all error messages with semicolons
    errors = fmt::format("{};", StringHelpers::join(error_messages, ";"));
    return false;
  }
  return true;
}

bool SpatialData::check_catalog(std::string &errors) {
  if (!using_raster_) { return true; }

  for (const auto &raster : data_) {
    if (!raster) { continue; }

    auto ref_raster_info = RasterInformation();
    ref_raster_info.number_columns = raster->ncols;
    ref_raster_info.number_rows = raster->nrows;
    ref_raster_info.x_lower_left_corner = raster->xllcorner;
    ref_raster_info.y_lower_left_corner = raster->yllcorner;
    ref_raster_info.cellsize = raster->cellsize;
    ref_raster_info.no_data_value = raster->nodata_value;

    spdlog::info("Checking raster: {}x{} with cell size: {}", ref_raster_info.number_columns,
                 ref_raster_info.number_rows, ref_raster_info.cellsize);

    if (ref_raster_info.number_columns == 0 || ref_raster_info.number_rows == 0
        || ref_raster_info.cellsize == 0) {
      return true;
    }

    if (!validate_raster_info(ref_raster_info, errors)) {
      errors = fmt::format("Header mismatch: {}", errors);
      return true;
    }
  }

  // check for all rasters have the same no_data cell locations
  AscFile* ref_raster = nullptr;
  for (const auto &raster : data_) {
    if (!raster) { continue; }
    // spdlog::info("Checking raster: {}x{} with cell size: {}", raster->ncols, raster->nrows,
    //              raster->cellsize);
    if (ref_raster == nullptr) {
      ref_raster = raster.get();
      continue;
    }

    for (int row = 0; row < raster->nrows; row++) {
      for (int col = 0; col < raster->ncols; col++) {
        if (raster->data[row][col] == raster->nodata_value) {
          if (ref_raster->data[row][col] != ref_raster->nodata_value) {
            errors = fmt::format("NODATA_VALUE mismatch: {}", raster->nodata_value);
            spdlog::error("Row {}, Col {}", row, col);
            spdlog::error("Raster error: {}", raster->nodata_value);
            return true;
          }
        }
      }
    }
  }

  return false;
}

void SpatialData::generate_distances() const {
  // Both the location database and distance storage belong to SpatialSettings.
  auto &db = spatial_settings_->location_db();

  auto provider = make_grid_distance_provider(db, cell_size_);
  const auto locations = provider->size();

  if (GRID_DISTANCE_BACKEND == GridDistanceBackend::LUT) {
    const auto* lut = dynamic_cast<const GridLutDistanceProvider*>(provider.get());
    if (lut == nullptr) {
      throw std::logic_error(
          "Raster-grid distance factory did not create the selected LUT backend");
    }

    const auto dense_bytes = locations * locations * sizeof(double);
    spdlog::info(
        "Euclidean distances for {} raster locations stored in {:.1f} MB (dense would be {:.1f} "
        "GB)",
        locations, lut->memory_bytes() / 1048576.0, dense_bytes / 1073741824.0);
  } else {
    const auto* dense = dynamic_cast<const DenseGridDistanceProvider*>(provider.get());
    if (dense == nullptr) {
      throw std::logic_error(
          "Raster-grid distance factory did not create the selected dense backend");
    }
    spdlog::debug("Updated Euclidean raster distances using the dense provider");
  }

  spatial_settings_->set_distance_provider(std::move(provider));
}

void SpatialData::generate_locations(AscFile* reference) {
  // Validate we found a reference raster
  if (reference == nullptr) {
    throw std::runtime_error("No spatial raster files available to generate locations");
  }

  // Using Raster Information to generate locations
  if (raster_info_.is_initialized()) {
    spdlog::info("Using Raster Information to generate locations");
  } else {
    throw std::runtime_error("Raster Information is not initialized");
  }
  // Pre-allocate the location database
  // location_db belongs to spatial settings
  auto &db = spatial_settings_->location_db();
  db.clear();

  // Calculate maximum possible size (all cells valid)
  const size_t max_size = static_cast<size_t>(raster_info_.number_rows)
                          * static_cast<size_t>(raster_info_.number_columns);
  db.reserve(max_size);

  // Generate locations for valid cells
  int location_id = 0;
  const double no_data = raster_info_.no_data_value;

  for (int row = 0; row < raster_info_.number_rows; ++row) {
    for (int col = 0; col < raster_info_.number_columns; ++col) {
      if (reference->data[row][col] == no_data) { continue; }
      db.emplace_back(Spatial::Location{.id = location_id++,
                                        .population_size = 0,
                                        .coordinate = {.latitude = static_cast<float>(row),
                                                       .longitude = static_cast<float>(col)}});
    }
  }

  // Reclaim excess memory
  db.shrink_to_fit();

  // Update the configured count
  spatial_settings_->set_number_of_locations(db.size());
  if (spatial_settings_->get_number_of_locations() == 0) {
    // This error should be redundant since the ASC loader should catch it
    spdlog::error("Zero locations loaded while parsing ASC file.");
  }
  const auto location_count = spatial_settings_->get_number_of_locations();

  if (location_count == 0) {
    throw std::runtime_error(fmt::format("No valid locations found in raster"));
  }
  auto no_data_count = max_size - location_count;
  spdlog::info("Generated {} locations from {} total cells, {} cells with no data", location_count,
               max_size, no_data_count);
}

SpatialData::RasterInformation SpatialData::get_raster_header() const { return raster_info_; }

std::optional<SpatialData::ValueRange> SpatialData::get_expected_value_range(SpatialFileType type) {
  switch (type) {
    // Probabilities, consumed directly by ProgressToClinicalEvent and
    // Mosquito::build_interrupted_feeding_indices without any rescaling.
    case SpatialFileType::PR_TREATMENT_UNDER5:
    case SpatialFileType::PR_TREATMENT_OVER5:
    case SpatialFileType::MOSQUITO_IFR:
      return ValueRange{.minimum = 0.0, .maximum = 1.0};

    // Everything else is unbounded as far as the loader is concerned.
    default:
      return std::nullopt;
  }
}

std::string_view SpatialData::get_type_name(SpatialFileType type) {
  switch (type) {
    case SpatialFileType::LOCATIONS:
      return "location";
    case SpatialFileType::POPULATION:
      return "population";
    case SpatialFileType::BETA:
      return "beta";
    case SpatialFileType::DISTRICTS:
      return "district";
    case SpatialFileType::TRAVEL:
      return "travel";
    case SpatialFileType::ECOCLIMATIC:
      return "ecoclimatic";
    case SpatialFileType::PR_TREATMENT_UNDER5:
      return "p_treatment_under_5";
    case SpatialFileType::PR_TREATMENT_OVER5:
      return "p_treatment_over_5";
    case SpatialFileType::MOSQUITO_SIZE:
      return "prmc_size";
    case SpatialFileType::MOSQUITO_IFR:
      return "interrupted_feeding_rate";
    default:
      return "unknown";
  }
}

void SpatialData::validate_value_range(const AscFile* raster, SpatialFileType type,
                                       const std::string &filename) {
  if (raster == nullptr) { return; }

  const auto range = get_expected_value_range(type);
  if (!range.has_value()) { return; }

  const double lower_bound = range->minimum - VALUE_RANGE_TOLERANCE;
  const double upper_bound = range->maximum + VALUE_RANGE_TOLERANCE;

  auto offending_cells = 0L;

  for (auto row = 0; row < raster->nrows; row++) {
    for (auto col = 0; col < raster->ncols; col++) {
      const float value = raster->data[row][col];
      if (value == raster->nodata_value) { continue; }
      if (value >= lower_bound && value <= upper_bound) { continue; }
      offending_cells++;
    }
  }

  if (offending_cells == 0) { return; }

  auto message =
      fmt::format("{} raster '{}' has {} cell(s) outside the required range [{}, {}].",
                  get_type_name(type), filename, offending_cells, range->minimum, range->maximum);

  spdlog::error(message);
  throw std::runtime_error(message);
}

void SpatialData::load(const std::string &filename, SpatialFileType type) {
  // No need to check and delete, unique_ptr handles it
  spdlog::info("Loading {}", filename);
  auto raster = AscFileManager::read(filename);

  // Range check before the raster is published to the catalog so that a
  // rejected file is never observable through get_raster().
  validate_value_range(raster.get(), type, filename);

  data_.at(type) = std::move(raster);
}

void SpatialData::populate_raster_data_to_location_db(SpatialFileType type) {
  // Verify that the raster data has been loaded
  if (!data_.at(type)) {
    throw std::runtime_error(fmt::format("{} called without raster, type id: {}", __FUNCTION__,
                                         static_cast<uint32_t>(type)));
  }

  // Get a reference to the raster for cleaner code
  AscFile* raster = data_.at(type).get();

  // Get a reference to the location database
  auto &db = spatial_settings_->location_db();
  auto count = spatial_settings_->get_number_of_locations();

  // Scan the data and update the values
  auto id = -1;
  for (auto ndx = 0; ndx < raster->nrows; ndx++) {
    for (auto ndy = 0; ndy < raster->ncols; ndy++) {
      if (raster->data[ndx][ndy] == raster->nodata_value) { continue; }
      id++;

      // Verify that we haven't exceeded our bounds
      if (id >= count) {
        throw std::runtime_error(
            fmt::format("{} found more locations than expected", __FUNCTION__));
      }

      // Update the appropriate value
      switch (type) {
        case SpatialFileType::BETA:
          db[id].beta = raster->data[ndx][ndy];
          break;
        case SpatialFileType::POPULATION:
          db[id].population_size = static_cast<int>(raster->data[ndx][ndy]);
          break;
        case SpatialFileType::PR_TREATMENT_UNDER5:
          db[id].p_treatment_under_5 = raster->data[ndx][ndy];
          break;
        case SpatialFileType::PR_TREATMENT_OVER5:
          db[id].p_treatment_over_5 = raster->data[ndx][ndy];
          break;
        default:
          break;
      }
    }
  }
}

void SpatialData::load_files(const YAML::Node &node) {
  using_raster_ = false;  // Reset flag at start

  if (node[LOCATION_RASTER]) {
    load(node[LOCATION_RASTER].as<std::string>(), SpatialData::SpatialFileType::LOCATIONS);
    using_raster_ = true;
  }
  if (node[BETA_RASTER]) {
    load(node[BETA_RASTER].as<std::string>(), SpatialData::SpatialFileType::BETA);
    using_raster_ = true;
  }
  if (node[POPULATION_RASTER]) {
    load(node[POPULATION_RASTER].as<std::string>(), SpatialData::SpatialFileType::POPULATION);
    using_raster_ = true;
  }
  if (node[TRAVEL_RASTER]) {
    load(node[TRAVEL_RASTER].as<std::string>(), SpatialData::SpatialFileType::TRAVEL);
    using_raster_ = true;
  }
  if (node[ECOCLIMATIC_RASTER]) {
    load(node[ECOCLIMATIC_RASTER].as<std::string>(), SpatialData::SpatialFileType::ECOCLIMATIC);
    using_raster_ = true;
  }
  if (node[TREATMENT_RATE_UNDER5]) {
    load(node[TREATMENT_RATE_UNDER5].as<std::string>(),
         SpatialData::SpatialFileType::PR_TREATMENT_UNDER5);
    using_raster_ = true;
  }
  if (node[TREATMENT_RATE_OVER5]) {
    load(node[TREATMENT_RATE_OVER5].as<std::string>(),
         SpatialData::SpatialFileType::PR_TREATMENT_OVER5);
    using_raster_ = true;
  }
  if (node[DISTRICT_RASTER]) {
    load(node[DISTRICT_RASTER].as<std::string>(), SpatialData::SpatialFileType::DISTRICTS);
    using_raster_ = true;
  }

  // Add support for the new administrative_boundaries section
  if (node[ADMIN_BOUNDARIES]) { load_admin_boundaries(node); }
  // Check to make sure our data is OK
  std::string errors;
  if (check_catalog(errors)) { throw std::runtime_error(errors); }
}

void SpatialData::load_admin_boundaries(const YAML::Node &node) {
  if (!node[ADMIN_BOUNDARIES].IsSequence()) {
    throw std::runtime_error("administrative_boundaries must be a sequence");
  }

  admin_rasters_.clear();

  // Process each administrative boundary
  for (const auto &admin_level : node[ADMIN_BOUNDARIES]) {
    if (!admin_level["name"] || !admin_level["raster"]) {
      throw std::runtime_error("Each administrative level must have a name and raster path");
    }

    auto level_name = admin_level["name"].as<std::string>();
    auto raster_path = admin_level["raster"].as<std::string>();

    // Store for processing later
    admin_rasters_[level_name] = raster_path;
    spdlog::info("Found admin level: {} with raster: {}", level_name, raster_path);
  }

  using_raster_ = true;
}

void SpatialData::load_age_distribution(const YAML::Node &node) {
  if (!node["age_distribution_by_location"]) {
    throw std::runtime_error("Missing required age distribution data");
  }

  auto &location_db = spatial_settings_->location_db();
  auto number_of_locations = spatial_settings_->get_number_of_locations();

  for (auto loc = 0; loc < number_of_locations; loc++) {
    auto input_loc = node["age_distribution_by_location"].size() < number_of_locations ? 0 : loc;
    const auto &age_dist = node["age_distribution_by_location"][input_loc];

    if (!age_dist.IsSequence()) { throw std::runtime_error("Age distribution must be a sequence"); }

    location_db[loc].age_distribution.clear();
    for (const auto &value : age_dist) {
      location_db[loc].age_distribution.push_back(value.as<double>());
    }
  }
}

void SpatialData::load_treatment_data(const YAML::Node &node) {
  auto &location_db = spatial_settings_->location_db();
  auto number_of_locations = spatial_settings_->get_number_of_locations();

  // Only load from YAML if raster not provided
  if (data_[SpatialFileType::PR_TREATMENT_UNDER5] != nullptr) {
    populate_raster_data_to_location_db(SpatialFileType::PR_TREATMENT_UNDER5);
  } else {
    if (!node["p_treatment_for_under_5_by_location"]) {
      throw std::runtime_error("Missing treatment rate data for under 5");
    }
    for (auto loc = 0; loc < number_of_locations; loc++) {
      auto input_loc =
          node["p_treatment_for_under_5_by_location"].size() < number_of_locations ? 0 : loc;
      location_db[loc].p_treatment_under_5 =
          node["p_treatment_for_under_5_by_location"][input_loc].as<float>();
    }
  }

  if (data_[SpatialFileType::PR_TREATMENT_OVER5] != nullptr) {
    populate_raster_data_to_location_db(SpatialFileType::PR_TREATMENT_OVER5);
  } else {
    if (!node["p_treatment_for_over_5_by_location"]) {
      throw std::runtime_error("Missing treatment rate data for over 5");
    }
    for (auto loc = 0; loc < number_of_locations; loc++) {
      auto input_loc =
          node["p_treatment_for_over_5_by_location"].size() < number_of_locations ? 0 : loc;
      location_db[loc].p_treatment_over_5 =
          node["p_treatment_for_over_5_by_location"][input_loc].as<float>();
    }
  }
}

void SpatialData::load_location_data(const YAML::Node &node) {
  auto &location_db = spatial_settings_->location_db();
  auto number_of_locations = spatial_settings_->get_number_of_locations();

  if (data_[SpatialFileType::BETA] != nullptr) {
    populate_raster_data_to_location_db(SpatialFileType::BETA);
  } else {
    if (!node["beta_by_location"]) { throw std::runtime_error("Missing beta data"); }
    for (auto loc = 0; loc < number_of_locations; loc++) {
      auto input_loc = node["beta_by_location"].size() < number_of_locations ? 0 : loc;
      location_db[loc].beta = node["beta_by_location"][input_loc].as<float>();
    }
  }
  if (data_[SpatialFileType::POPULATION] != nullptr) {
    populate_raster_data_to_location_db(SpatialFileType::POPULATION);
  } else {
    if (!node["population_size_by_location"]) {
      throw std::runtime_error("Missing population data");
    }
    for (auto loc = 0; loc < number_of_locations; loc++) {
      auto input_loc = node["population_size_by_location"].size() < number_of_locations ? 0 : loc;
      location_db[loc].population_size = node["population_size_by_location"][input_loc].as<int>();
      spdlog::info("Location: {}, Population: {}", loc, location_db[loc].population_size);
    }
  }
}

void SpatialData::initialize_admin_boundaries() {
  // Create a new AdminLevelManager
  admin_manager_ = std::make_unique<AdminLevelManager>();

  if (!using_raster_) { return; }
  if (admin_rasters_.empty()) {
    // there will be cases where we don't need to have any admin levels
    return;
  }

  // Now process all other admin levels
  for (const auto &[level_name, raster_path] : admin_rasters_) {
    try {
      admin_manager_->register_level(level_name);

      // Load the raster
      auto raster = std::unique_ptr<AscFile>(AscFileManager::read(raster_path));
      admin_manager_->setup_boundary(level_name, raster.get());

      spdlog::info("Initialized admin level: {}", level_name);
    } catch (const std::exception &e) {
      spdlog::error("Failed to initialize admin level {}: {}", level_name, e.what());
      throw;
    }
  }

  // Validate the configuration
  try {
    admin_manager_->validate();
  } catch (const std::exception &e) {
    spdlog::error("AdminLevelManager validation failed: {}", e.what());
    throw std::runtime_error(e.what());
  }
  spdlog::info("Administrative boundaries initialized successfully");
}

void SpatialData::parse_complete() {
  // Simply reset unique_ptrs instead of manual delete
  // some rasters are not reset because they are used by other components for initialization
  // i.e: SeasonalImmunity reporter requires the Ecoclimatic raster
  // SeasonalEquation reporter requires the Ecoclimatic raster
  // SpatialModel requires the Travel raster
  // ...

  data_[SpatialFileType::BETA].reset();
  data_[SpatialFileType::POPULATION].reset();
  data_[SpatialFileType::PR_TREATMENT_UNDER5].reset();
  data_[SpatialFileType::PR_TREATMENT_OVER5].reset();

  // Note: We don't reset Districts raster as ownership might have been transferred
  // Similarly, we don't need to reset any admin rasters as they are stored in custom objects

  // Clean up the temporary admin rasters map as it's no longer needed
  admin_rasters_.clear();
}
