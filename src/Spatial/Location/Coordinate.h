#ifndef SPATIAL_COORDINATE_H
#define SPATIAL_COORDINATE_H

#include <cmath>
#include <numbers>
#include <ostream>

namespace Spatial {
struct Coordinate {
public:
  float latitude;
  float longitude;

  static double calculate_distance_in_km(const Coordinate &from, const Coordinate &to) {
    // using Haversine
    constexpr double PI = std::numbers::pi_v<double> / 180;
    constexpr int EARTH_RADIUS = 6371;  // Radius of the Earth in km
    double d_lat = PI * (from.latitude - to.latitude);
    double d_lon = PI * (from.longitude - to.longitude);
    double aa =
        (std::sin(d_lat / 2) * std::sin(d_lat / 2))
        + (std::cos(from.latitude * PI) * std::cos(to.latitude * PI) * std::sin(d_lon / 2)
           * std::sin(d_lon / 2));
    double cc = 2 * std::atan2(std::sqrt(aa), std::sqrt(1 - aa));
    double result = EARTH_RADIUS * cc;

    return result;
  }

  friend std::ostream &operator<<(std::ostream &os, const Coordinate &coordinate) {
    os << "[latitude: " << coordinate.latitude << " - longitude: " << coordinate.longitude << "]";
    return os;
  }
};
}  // namespace Spatial

#endif  // SPATIAL_COORDINATE_H
