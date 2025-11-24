#ifndef SEASONALITYSETTINGS_H
#define SEASONALITYSETTINGS_H

#include <Utils/Helpers/TimeHelpers.h>
#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "Environment/SeasonalEquation.h"
#include "Environment/SeasonalPattern.h"
#include "Environment/SeasonalRainfall.h"
#include "IConfigData.h"
#include "Spatial/GIS/SpatialData.h"
#include "date/date.h"

// Class to hold seasonality settings, combining RainfallSettings and
// SimpleSettings
class SeasonalitySettings : public IConfigData {
public:
  SeasonalitySettings(const SeasonalitySettings &) = delete;
  SeasonalitySettings &operator=(const SeasonalitySettings &) = delete;
  SeasonalitySettings(SeasonalitySettings &&) = default;
  SeasonalitySettings &operator=(SeasonalitySettings &&) = default;

  SeasonalitySettings();
  ~SeasonalitySettings() override;
  // Getters
  [[nodiscard]] bool get_enable() const { return enable_; }

  // Setters
  void set_enable(bool value) { enable_ = value; }

  [[nodiscard]] std::string get_mode() const { return mode_; }

  void set_mode(std::string value) { mode_ = value; }

  [[nodiscard]] SeasonalPattern* get_seasonal_pattern() const { return seasonal_pattern_.get(); }

  void set_seasonal_pattern(std::unique_ptr<SeasonalPattern> value) {
    seasonal_pattern_ = std::move(value);
  }

  [[nodiscard]] SeasonalRainfall* get_seasonal_rainfall() const { return seasonal_rainfall_.get(); }

  /**
   * @brief Set the seasonal rainfall object
   *
   * @param value A unique pointer to a SeasonalRainfall object
   *
   * This function sets the seasonal rainfall object which is used to calculate
   * the seasonal factor for a given date and location.
   */
  void set_seasonal_rainfall(std::unique_ptr<SeasonalRainfall> value) {
    seasonal_rainfall_ = std::move(value);
  }

  [[nodiscard]] SeasonalEquation* get_seasonal_equation() const { return seasonal_equation_.get(); }

  void set_seasonal_equation(std::unique_ptr<SeasonalEquation> value) {
    seasonal_equation_ = std::move(value);
  }

  [[nodiscard]] double get_seasonal_factor(const date::sys_days &today, const int &location) {
    if (enable_) {
      if (mode_ == "equation") {
        return get_seasonal_equation()->get_seasonal_factor(today, location);
      }
      if (mode_ == "rainfall") {
        return get_seasonal_rainfall()->get_seasonal_factor(today, location);
      }
      if (mode_ == "pattern") {
        return get_seasonal_pattern()->get_seasonal_factor(today, location);
      }
    }
    return 1.0;
  }

  void process_config() override {}

  void process_config_using_number_of_locations(SpatialData* spatial_data,
                                                size_t number_of_locations) {
    spdlog::info("Processing SeasonalitySettings");
    if (enable_) {
      spdlog::info("Seasonality enabled");
      if (mode_ == "equation") {
        spdlog::info("Equation based");
        // process equation based
        seasonal_equation_->build(number_of_locations);
      } else if (mode_ == "rainfall") {
        spdlog::info("Rainfall based");
        // process rainfall based
        seasonal_rainfall_->build();
      } else if (mode_ == "pattern") {
        spdlog::info("Pattern based");
        // process rainfall based
        seasonal_pattern_->build(spatial_data);
      } else {
        throw std::runtime_error("Unknown mode in 'seasonality_settings'.");
      }
    } else {
      spdlog::info("Seasonality disabled, using default value of 1.0");
    }
  }

private:
  bool enable_ = false;
  std::string mode_;
  std::unique_ptr<SeasonalEquation> seasonal_equation_{nullptr};
  std::unique_ptr<SeasonalRainfall> seasonal_rainfall_{nullptr};
  std::unique_ptr<SeasonalPattern> seasonal_pattern_{nullptr};
};

namespace YAML {
// convert specialization for SeasonalitySettings::SeasonalEquation
template <>
struct convert<SeasonalEquation*> {
  static Node encode(const SeasonalEquation* rhs) {
    Node node;
    node["raster"] = rhs->get_raster();
    node["base"] = rhs->get_raster_base();
    node["a"] = rhs->get_raster_A();
    node["b"] = rhs->get_raster_B();
    node["phi"] = rhs->get_raster_phi();
    return node;
  }

  static bool decode(const Node &node, SeasonalEquation* rhs) {
    if (!node["base"] || !node["a"] || !node["b"] || !node["phi"] || !node["raster"]) {
      throw std::runtime_error("Missing fields in SeasonalEquation");
    }
    rhs->set_raster(node["raster"].as<bool>());
    rhs->set_raster_base(node["base"].as<std::vector<double>>());
    rhs->set_raster_A(node["a"].as<std::vector<double>>());
    rhs->set_raster_B(node["b"].as<std::vector<double>>());
    rhs->set_raster_phi(node["phi"].as<std::vector<int>>());
    rhs->set_A(rhs->get_raster_A());
    rhs->set_B(rhs->get_raster_B());
    rhs->set_base(rhs->get_raster_base());
    rhs->set_phi(rhs->get_raster_phi());
    rhs->set_reference_A(rhs->get_raster_A());
    rhs->set_reference_B(rhs->get_raster_B());
    rhs->set_reference_base(rhs->get_raster_base());
    rhs->set_reference_phi(rhs->get_raster_phi());

    spdlog::info("SeasonalEquation initialized {}", rhs->get_raster_A().back());
    return true;
  }
};

// Convert specialization for SeasonalitySettings::SeasonalRainfall
template <>
struct convert<SeasonalRainfall*> {
  static Node encode(const SeasonalRainfall* rhs) {
    Node node;
    node["filename"] = rhs->get_filename();
    node["period"] = rhs->get_period();
    return node;
  }

  static bool decode(const Node &node, SeasonalRainfall* rhs) {
    if (!node["filename"] || !node["period"]) {
      throw std::runtime_error("Missing fields in SeasonalRainfall");
    }
    rhs->set_filename(node["filename"].as<std::string>());
    rhs->set_period(node["period"].as<int>());
    return true;
  }
};

// Convert specialization for SeasonalitySettings::SeasonalRainfall
template <>
struct convert<SeasonalPattern*> {
  static Node encode(const SeasonalPattern* rhs) {
    Node node;
    node["filename"] = rhs->get_filename();
    node["period"] = rhs->get_period();
    node["admin_level"] = rhs->get_admin_level();
    return node;
  }

  static bool decode(const Node &node, SeasonalPattern* rhs) {
    if (!node["filename"] || !node["period"] || !node["admin_level"]) {
      throw std::runtime_error("Missing fields in SeasonalPattern");
    }
    rhs->set_filename(node["filename"].as<std::string>());
    rhs->set_period(node["period"].as<int>());
    if (rhs->get_period() != 12 && rhs->get_period() != 365) {
      throw std::invalid_argument("Period must be either 12 (monthly) or 365 (daily)");
    }
    rhs->set_admin_level(node["admin_level"].as<std::string>());
    rhs->set_is_monthly(rhs->get_period() == 12);
    return true;
  }
};

// Convert specialization for SeasonalitySettings
template <>
struct convert<SeasonalitySettings> {
  static Node encode(const SeasonalitySettings &rhs) {
    Node node;
    node["enable"] = rhs.get_enable();
    node["mode"] = rhs.get_mode();
    node["equation"]["base"] = rhs.get_seasonal_equation()->get_base();
    node["equation"]["a"] = rhs.get_seasonal_equation()->get_A();
    node["equation"]["b"] = rhs.get_seasonal_equation()->get_B();
    node["equation"]["phi"] = rhs.get_seasonal_equation()->get_phi();
    node["rainfall"]["filename"] = rhs.get_seasonal_rainfall()->get_filename();
    node["rainfall"]["period"] = rhs.get_seasonal_rainfall()->get_period();
    return node;
  }

  static bool decode(const Node &node, SeasonalitySettings &rhs) {
    if (!node["enable"]) {
      throw std::runtime_error("Missing field enable in SeasonalitySettings");
    }

    rhs.set_enable(node["enable"].as<bool>());

    if (!node["mode"]) {
      throw std::runtime_error("Missing fields mode in SeasonalitySettings when enable is true");
    }
    rhs.set_mode(node["mode"].as<std::string>());

    if (!node["equation"] && !node["rainfall"] && !node["pattern"]) {
      throw std::runtime_error(
          "Missing equation/rainfall/pattern node in SeasonalitySettings when "
          "mode is set");
    }

    if (rhs.get_mode() == "equation") {
      auto model = std::make_unique<SeasonalEquation>();
      convert<SeasonalEquation*>::decode(node["equation"], model.get());
      rhs.set_seasonal_equation(std::move(model));
    }
    if (rhs.get_mode() == "rainfall") {
      auto model = std::make_unique<SeasonalRainfall>();
      convert<SeasonalRainfall*>::decode(node["rainfall"], model.get());
      rhs.set_seasonal_rainfall(std::move(model));
    }
    if (rhs.get_mode() == "pattern") {
      auto model = std::make_unique<SeasonalPattern>();
      convert<SeasonalPattern*>::decode(node["pattern"], model.get());
      rhs.set_seasonal_pattern(std::move(model));
    }
    return true;
  }
};
}  // namespace YAML
#endif  // SEASONALITYSETTINGS_H
