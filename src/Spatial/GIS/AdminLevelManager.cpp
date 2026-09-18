#include "AdminLevelManager.h"

#include <spdlog/spdlog.h>

#include <set>
#include <stdexcept>

#include "Core/types.h"

int AdminLevelManager::register_level(const std::string &name) {
  // Check if name already exists
  if (has_level(name)) {
    throw std::runtime_error("Administrative level '" + name + "' already exists");
  }

  // Register the new level
  int id = static_cast<int>(id_to_name_.size());
  id_to_name_.push_back(name);
  name_to_id_[name] = id;
  boundaries_.emplace_back();

  return id;
}

void AdminLevelManager::set_boundary(int level_id, const BoundaryData &in_boundary) {
  if (level_id < 0 || level_id >= static_cast<int>(boundaries_.size())) {
    throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
  }

  auto &boundary = boundaries_[level_id];
  boundary.location_to_unit = in_boundary.location_to_unit;
  boundary.unit_to_locations = in_boundary.unit_to_locations;
  boundary.min_unit_id = in_boundary.min_unit_id;
  boundary.max_unit_id = in_boundary.max_unit_id;
  boundary.unit_count = in_boundary.unit_count;

  auto name = id_to_name_[level_id];
  // Validate the boundary data
  if (boundary.unit_count == 0) {
    throw std::runtime_error("Administrative level '" + name + "' has no units");
  }

  if (boundary.min_unit_id != 0 && boundary.min_unit_id != 1) {
    throw std::runtime_error("Administrative level '" + name
                             + "' has invalid minimum unit ID of 0 or 1");
  }

  if (boundary.unit_count != boundary.max_unit_id - boundary.min_unit_id + 1) {
    throw std::runtime_error("Administrative level '" + name + "' has invalid unit ID range");
  }

  spdlog::info("Initialized administrative level '{}' with {} units", name, boundary.unit_count);
}

void AdminLevelManager::setup_boundary(const std::string &name, AscFile* raster) {
  auto it = name_to_id_.find(name);
  if (it == name_to_id_.end()) {
    throw std::runtime_error("Administrative level '" + name + "' not registered");
  }
  // Validate raster
  if (raster == nullptr) { throw std::runtime_error("Null raster provided for '" + name + "'"); }
  validate_raster(raster);

  auto result = populate_lookup(raster);

  // Set up boundary data
  set_boundary(it->second, result);
}

int AdminLevelManager::get_admin_level_id(const std::string &level_name) const {
  auto it = name_to_id_.find(level_name);
  if (it == name_to_id_.end()) {
    spdlog::error("Administrative level '" + level_name + "' not found");
    throw std::runtime_error("Administrative level '" + level_name + "' not found");
  }
  return it->second;
}

int AdminLevelManager::get_admin_unit(const std::string &level_name,
                                      core::LocationId location) const {
  auto it = name_to_id_.find(level_name);
  if (it == name_to_id_.end()) {
    throw std::runtime_error("get_admin_unit: Administrative level '" + level_name + "' not found");
  }
  return get_admin_unit(it->second, location);
}

int AdminLevelManager::get_admin_unit(int level_id, core::LocationId location) const {
  if (level_id < 0 || level_id >= static_cast<int>(boundaries_.size())) {
    throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
  }
  if (location < 0 || location >= static_cast<int>(boundaries_[level_id].location_to_unit.size())) {
    throw std::out_of_range("Invalid location ID: " + std::to_string(location));
  }
  return boundaries_[level_id].location_to_unit[location];
}

const std::vector<int> &AdminLevelManager::get_locations_in_unit(const std::string &level_name,
                                                                 int unit_id) const {
  auto it = name_to_id_.find(level_name);
  if (it == name_to_id_.end()) {
    throw std::runtime_error("get_locations_in_unit: Administrative level '" + level_name
                             + "' not found");
  }
  return get_locations_in_unit(it->second, unit_id);
}

const std::vector<int> &AdminLevelManager::get_locations_in_unit(int level_id, int unit_id) const {
  if (level_id < 0 || level_id >= static_cast<int>(boundaries_.size())) {
    throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
  }
  if (unit_id < 0 || unit_id >= static_cast<int>(boundaries_[level_id].unit_to_locations.size())) {
    throw std::out_of_range("Invalid unit ID: " + std::to_string(unit_id));
  }
  return boundaries_[level_id].unit_to_locations[unit_id];
}

std::pair<int, int> AdminLevelManager::get_units(const std::string &level_name) const {
  auto it = name_to_id_.find(level_name);
  if (it == name_to_id_.end()) {
    throw std::runtime_error("get_units: Administrative level '" + level_name + "' not found");
  }
  return {boundaries_[it->second].min_unit_id, boundaries_[it->second].max_unit_id};
}

const BoundaryData* AdminLevelManager::get_boundary(const std::string &name) const {
  auto it = name_to_id_.find(name);
  return it != name_to_id_.end() ? &boundaries_[it->second] : nullptr;
}

int AdminLevelManager::get_unit_count(const std::string &level_name) const {
  auto it = name_to_id_.find(level_name);
  if (it == name_to_id_.end()) {
    throw std::runtime_error("get_unit_count: Administrative level '" + level_name + "' not found");
  }
  return get_unit_count(it->second);
}

int AdminLevelManager::get_unit_count(int level_id) const {
  if (level_id < 0 || level_id >= static_cast<int>(boundaries_.size())) {
    spdlog::info("Invalid level ID: {}", level_id);
    spdlog::info("Boudaries size: {}", boundaries_.size());
    throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
  }
  return boundaries_[level_id].unit_count;
}

BoundaryData AdminLevelManager::populate_lookup(const AscFile* raster) {
  BoundaryData result;

  auto min_unit_id = std::numeric_limits<int>::max();
  auto max_unit_id = std::numeric_limits<int>::min();

  // Perform a consistency check on the districts
  std::set<int> unique_unit_id;

  auto location_count = 0;
  for (auto ndx = 0; ndx < raster->nrows; ndx++) {
    for (auto ndy = 0; ndy < raster->ncols; ndy++) {
      auto value = raster->data[ndx][ndy];
      if (value == raster->nodata_value) { continue; }
      auto unit_id = static_cast<int>(value);
      unique_unit_id.insert(unit_id);
      min_unit_id = std::min(min_unit_id, unit_id);
      max_unit_id = std::max(max_unit_id, unit_id);
      location_count++;
    }
  }

  if (unique_unit_id.size() != max_unit_id - min_unit_id + 1) {
    throw std::runtime_error("Invalid unit ID range in raster");
  }

  result.location_to_unit.clear();
  result.location_to_unit.resize(location_count);
  result.unit_to_locations.clear();
  result.unit_to_locations.resize(max_unit_id + 1);  // handle both 0 and 1 based indexing

  auto location_id = 0;
  for (auto ndx = 0; ndx < raster->nrows; ndx++) {
    for (auto ndy = 0; ndy < raster->ncols; ndy++) {
      auto value = raster->data[ndx][ndy];
      if (value == raster->nodata_value) { continue; }
      auto unit_id = static_cast<int>(value);
      result.location_to_unit[location_id] = unit_id;
      result.unit_to_locations[unit_id].push_back(location_id);
      location_id++;
    }
  }

  result.min_unit_id = min_unit_id;
  result.max_unit_id = max_unit_id;
  result.unit_count = static_cast<int>(unique_unit_id.size());

  return result;
}

void AdminLevelManager::validate_raster(const AscFile* raster) {
  if (raster == nullptr) { throw std::runtime_error("Null raster provided"); }

  if (raster->nrows <= 0 || raster->ncols <= 0) {
    throw std::runtime_error("Invalid raster dimensions");
  }
  // TODO: check if the raster is a valid admin level raster
}

void AdminLevelManager::validate() const {
  // Validate each boundary in id_to_name
  for (int i = 0; i < id_to_name_.size(); i++) {
    if (boundaries_[i].unit_count == 0) {
      throw std::runtime_error("Administrative level '" + id_to_name_[i] + "' has no units");
    }
  }

  // all admin levels must have the same dimensions for location_to_unit
  for (const auto &boundarie : boundaries_) {
    if (boundarie.location_to_unit.size() != boundaries_[0].location_to_unit.size()) {
      throw std::runtime_error(
          "All admin levels must have the same dimensions for location_to_unit");
    }
    // each unit_to_locations must have the size of max_unit_id + 1
    if (boundarie.unit_to_locations.size() != boundarie.max_unit_id + 1) {
      throw std::runtime_error("unit_to_locations must have the size of max_unit_id + 1");
    }

    // each unit_count should be max_unit_id - min_unit_id + 1
    if (boundarie.unit_count != boundarie.max_unit_id - boundarie.min_unit_id + 1) {
      throw std::runtime_error("unit_count should be max_unit_id - min_unit_id + 1");
    }
  }
}
