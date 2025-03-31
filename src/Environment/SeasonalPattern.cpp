#include "SeasonalPattern.h"
#include <fstream>
#include "Spatial/GIS/SpatialData.h"

void SeasonalPattern::build(SpatialData* spatial_data) {
  admin_level_id = spatial_data->get_admin_level_id(admin_level);
  if (admin_level_id == -1) {
    throw std::invalid_argument("The admin level parameter is invalid.");
  }
  read(filename);

  // Validate against SpatialData if available
  if (spatial_data->get_unit_count(admin_level_id) > 0) {
    const auto *boundary = spatial_data->get_boundary(admin_level);
    if (admin_unit_adjustments.size()
        != boundary->max_unit_id + 1) {
      throw std::runtime_error(
          fmt::format("Expected {} {}s, got {}",
                      boundary->max_unit_id + 1,
                      admin_level,
                      admin_unit_adjustments.size()));
        }
  }
}

int SeasonalPattern::get_admin_unit_for_location(const int location) const {
  if (SpatialData::get_instance().get_unit_count(admin_level_id) <= 0) {
    return min_admin_unit_id;
  }

  return SpatialData::get_instance().get_admin_unit(admin_level_id, location);
}

void SeasonalPattern::read(const std::string &filename) {
    std::ifstream in(filename);
    if (!in.good()) {
        throw std::runtime_error("Error opening the seasonal pattern file: " + filename);
    }
    std::string line;
    std::getline(in, line);
    min_admin_unit_id = std::numeric_limits<int>::max();
    max_admin_unit_id = std::numeric_limits<int>::min();
    std::map<int, std::vector<double>> temp_adjustments;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ',');
        int admin_unit_id = std::stoi(token);
        min_admin_unit_id = std::min(min_admin_unit_id, admin_unit_id);
        max_admin_unit_id = std::max(max_admin_unit_id, admin_unit_id);
        std::vector<double> factors;
        while (std::getline(ss, token, ',')) {
            double factor = std::stod(token);
            if (factor < 0.0) {
                throw std::runtime_error("Seasonal factor less than zero: " + std::to_string(factor));
            }
            factors.push_back(factor);
        }
        if (factors.size() != period) {
            throw std::runtime_error("Incorrect number of seasonal factors in file.");
        }
        temp_adjustments[admin_unit_id] = factors;
    }

    // Determine if input is 0-based or 1-based
    bool is_one_based = (min_admin_unit_id == 1);
    bool is_zero_based = (min_admin_unit_id == 0);

    if (!is_one_based && !is_zero_based) {
      throw std::runtime_error(fmt::format(
          "Admin unit IDs must start at 0 or 1, but found minimum ID: {}",
          min_admin_unit_id));
    }

    // Calculate actual admin unit count
    int actual_admin_unit_count = max_admin_unit_id - min_admin_unit_id + 1;

    // Size the vector to accommodate direct indexing (size = count for 0-based, count+1 for 1-based)
    admin_unit_adjustments.clear();
    admin_unit_adjustments.resize(min_admin_unit_id == 0 ? actual_admin_unit_count : actual_admin_unit_count + 1);

    // Store factors using original admin unit IDs directly
    for (const auto &[file_id, factors] : temp_adjustments) {
      admin_unit_adjustments[file_id] = factors;
    }
  spdlog::info("Loaded {} admin units from {} ({}-based indexing)",
                         actual_admin_unit_count, filename,
                         min_admin_unit_id);
}

// int SeasonalPattern::get_district_for_location(int location) const {
//     if (SpatialData::get_instance().district_count == -1) {
//         return min_admin_unit_id;
//     }
//     return SpatialData::get_instance().get_district(location);
// }

double SeasonalPattern::get_seasonal_factor(const date::sys_days &today, const int &location) {
  const int admin_unit = get_admin_unit_for_location(location);

  int doy = TimeHelpers::day_of_year(today);

  // Get the month (0-11)
  const auto ymd = date::year_month_day{today};
  const auto month = static_cast<unsigned int>(ymd.month()) - 1;

  // For monthly data, use the month directly
  if (is_monthly) {
    return admin_unit_adjustments[admin_unit][month];
  }
  // For daily data, use the day of year (0-364)
  doy = doy == 366 ? 364 : doy - 1;
  return admin_unit_adjustments[admin_unit][doy];
}