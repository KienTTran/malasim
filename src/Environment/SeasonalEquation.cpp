/*
 * SeasonalEquation.cpp
 *
 * Implement the equation based seasonal model.
 */

#include "SeasonalEquation.h"

#include <cstdint>

#include "Simulation/Model.h"

SeasonalEquation::SeasonalEquation() = default;

void SeasonalEquation::build(int number_of_locations) {
  if (raster_) { set_from_raster(); }
  if (A_.size() > 1 && A_.size() < number_of_locations) {
    spdlog::info("Only {} seasonal equation settings provided, but {} are needed for all locations",
                 A_.size(), number_of_locations);
  }
  for (auto i = 0; i < number_of_locations; i++) {
    auto input_loc = A_.size() < number_of_locations ? 0 : i;
    set_seasonal_period(input_loc);
  }
}

double SeasonalEquation::get_seasonal_factor(const date::sys_days &today, const int &location) {
  int day = TimeHelpers::day_of_year(today);
  double multiplier = A_[location]
                      * sin(B_[location] * M_PI * (day - phi_[location])
                            / static_cast<double>(Constants::DAYS_IN_YEAR));
  multiplier = (multiplier < 0) ? 0 : multiplier;
  multiplier += base_[location];
  return multiplier;
}

void SeasonalEquation::set_from_raster() {
  AscFile* raster =
      Model::get_spatial_data()->get_raster(SpatialData::SpatialFileType::ECOCLIMATIC);
  if (raster == nullptr) { throw std::invalid_argument("Ecoclimatic raster not found."); }
  spdlog::info("Setting seasonal equation using raster data.");
  auto size = A_.size();
  for (int row = 0; row < raster->nrows; row++) {
    for (int col = 0; col < raster->ncols; col++) {
      if (raster->data[row][col] == raster->nodata_value) continue;
      int zone_index = static_cast<int>(raster->data[row][col]);
      if (zone_index < 0 || zone_index >= size) continue;
      set_seasonal_period(zone_index);
    }
  }
}

void SeasonalEquation::set_seasonal_period(uint64_t index) {
  base_.push_back(raster_base_[index]);
  A_.push_back(raster_A_[index]);
  B_.push_back(raster_B_[index]);
  phi_.push_back(raster_phi_[index]);
  reference_base_.push_back(raster_base_[index]);
  reference_A_.push_back(raster_A_[index]);
  reference_B_.push_back(raster_B_[index]);
  reference_phi_.push_back(raster_phi_[index]);
}

void SeasonalEquation::update_seasonality(int from, int to) {
  for (size_t ndx = 0; ndx < base_.size(); ndx++) {
    if (base_[ndx] == reference_base_[from] && A_[ndx] == reference_A_[from]
        && B_[ndx] == reference_B_[from] && phi_[ndx] == reference_phi_[from]) {
      base_[ndx] = reference_base_[to];
      A_[ndx] = reference_A_[to];
      B_[ndx] = reference_B_[to];
      phi_[ndx] = reference_phi_[to];
    }
  }
}
