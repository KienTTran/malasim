#include "SeasonalRainfall.h"

void SeasonalRainfall::build() {
  read(filename_);
  if (adjustments_.size() != period_) {
    throw std::invalid_argument(fmt::format(
        "The number of rainfall data points ({}) should match the period ({}).",
        adjustments_.size(), period_));
  }
}

double SeasonalRainfall::get_seasonal_factor(const date::sys_days &today, const int &location) {
  int doy = TimeHelpers::day_of_year(today);
  doy = doy == 366 ? doy - 2 : doy - 1;
  return adjustments_[doy];
}

void SeasonalRainfall::read(const std::string &filename) {
  std::ifstream in(filename);
  if (!in.good()) {
    throw std::runtime_error("Error opening the rainfall data file: " + filename);
  }
  if (in.peek() == std::ifstream::traits_type::eof()) {
    throw std::runtime_error("EOF encountered at start of rainfall data file: " + filename);
  }
  double data = 0.0;
  while (in >> data) {
    if (data > 1.0) {
      throw std::runtime_error(fmt::format("Rainfall factor exceeded 1.0: {0}", data));
    }
    if (data < 0.0) {
      throw std::runtime_error(fmt::format("Rainfall factor less than zero: {0}", data));
    }
    adjustments_.emplace_back(data);
  }
}

std::string SeasonalRainfall::get_filename() const {
  return filename_;
}

void SeasonalRainfall::set_filename(const std::string &value) {
  filename_ = value;
}

int SeasonalRainfall::get_period() const {
  return period_;
}

void SeasonalRainfall::set_period(const int value) {
  period_ = value;
}