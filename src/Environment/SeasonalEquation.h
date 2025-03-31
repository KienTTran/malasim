#ifndef SEASONALEQUATION_H
#define SEASONALEQUATION_H

#include "SeasonalInfo.h"
#include "Spatial/GIS/SpatialData.h"
#include "Utils/Helpers/TimeHelpers.h"

class SeasonalEquation : public ISeasonalInfo {
public:
  ~SeasonalEquation() override = default;
  SeasonalEquation() = default;

  void build(int number_of_locations);
  double get_seasonal_factor(const date::sys_days &today, const int &location) override;
  void set_from_raster();
  void set_seasonal_period(unsigned long index);
  void update_seasonality(int from, int to);

  [[nodiscard]] bool get_raster() const { return raster_; }
  void set_raster(bool value) { raster_ = value; }

  [[nodiscard]] std::string get_ecoclimatic_raster() const { return ecoclimatic_raster_; }
  void set_ecoclimatic_raster(const std::string &value) { ecoclimatic_raster_ = value; }

  [[nodiscard]] std::vector<double> get_raster_base() const { return raster_base_; }
  void set_raster_base(const std::vector<double> &value) { raster_base_ = value; }

  [[nodiscard]] std::vector<double> get_raster_A() const { return raster_A_; }
  void set_raster_A(const std::vector<double> &value) { raster_A_ = value; }

  [[nodiscard]] std::vector<double> get_raster_B() const { return raster_B_; }
  void set_raster_B(const std::vector<double> &value) { raster_B_ = value; }

  [[nodiscard]] std::vector<int> get_raster_phi() const { return raster_phi_; }
  void set_raster_phi(const std::vector<int> &value) { raster_phi_ = value; }

  [[nodiscard]] std::vector<double> get_base() const { return base_; }
  void set_base(const std::vector<double> &value) { base_ = value; }

  [[nodiscard]] std::vector<double> get_A() const { return A_; }
  void set_A(const std::vector<double>& value) { A_ = value; }

  [[nodiscard]] std::vector<double> get_B() const { return B_; }
  void set_B(const std::vector<double>& value) { B_ = value; }

  [[nodiscard]] std::vector<int> get_phi() const { return phi_; }
  void set_phi(const std::vector<int> &value) { phi_ = value; }

  [[nodiscard]] std::vector<double> get_reference_base() const { return reference_base_; }
  void set_reference_base(const std::vector<double> &value) { reference_base_ = value; }

  [[nodiscard]] std::vector<double> get_reference_A() const { return reference_A_; }
  void set_reference_A(const std::vector<double> &value) { reference_A_ = value; }

  [[nodiscard]] std::vector<double> get_reference_B() const { return reference_B_; }
  void set_reference_B(const std::vector<double> &value) { reference_B_ = value; }

  [[nodiscard]] std::vector<int> get_reference_phi() const { return reference_phi_; }
  void set_reference_phi(const std::vector<int> &value) { reference_phi_ = value; }

private:
  bool raster_ = true;
  std::string ecoclimatic_raster_;
  std::vector<double> raster_base_, raster_A_, raster_B_;
  std::vector<int> raster_phi_;
  std::vector<double> base_, A_, B_;
  std::vector<int> phi_;
  std::vector<double> reference_base_, reference_A_, reference_B_;
  std::vector<int> reference_phi_;
};

#endif 