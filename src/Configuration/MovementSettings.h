#include <gsl/gsl_cdf.h>
#include <yaml-cpp/node/node.h>

#include <stdexcept>
#include <string>

#include "IConfigData.h"
#include "Spatial/Movement/BarabasiSM.hxx"
#include "Spatial/Movement/BurkinaFasoSM.h"
#include "Spatial/Movement/MarshallSM.hxx"
#include "Spatial/Movement/WesolowskiSM.hxx"
#include "Spatial/Movement/WesolowskiSurfaceSM.h"
#include "Utils/MultinomialDistributionGenerator.h"
#include "spdlog/spdlog.h"

class MultinomialDistributionGenerator;

class MovementSettings : public IConfigData {
public:
  class BarabasiSM {
  public:
    // Getters and Setters
    [[nodiscard]] double get_r_g_0() const { return r_g_0_; }
    void set_r_g_0(const double value) { r_g_0_ = value; }

    [[nodiscard]] double get_beta_r() const { return beta_r_; }
    void set_beta_r(const double value) { beta_r_ = value; }

    [[nodiscard]] int get_kappa() const { return kappa_; }
    void set_kappa(const int value) {
      if (value <= 0) throw std::invalid_argument("kappa must be positive");
      kappa_ = value;
    }

  private:
    double r_g_0_ = 5.8;
    double beta_r_ = 1.65;
    int kappa_ = 350;
  };

  class WesolowskiSM {
  public:
    // Getters and Setters
    [[nodiscard]] double get_kappa() const { return kappa_; }
    void set_kappa(const double value) { kappa_ = value; }

    [[nodiscard]] double get_alpha() const { return alpha_; }
    void set_alpha(const double value) { alpha_ = value; }

    [[nodiscard]] double get_beta() const { return beta_; }
    void set_beta(const double value) { beta_ = value; }

    [[nodiscard]] double get_gamma() const { return gamma_; }
    void set_gamma(const double value) { gamma_ = value; }

  private:
    double kappa_ = 0.01093251;
    double alpha_ = 0.22268982;
    double beta_ = 0.14319618;
    double gamma_ = 0.83741484;
  };

  class WesolowskiSurfaceSM {
  public:
    // Getters and Setters
    [[nodiscard]] double get_kappa() const { return kappa_; }
    void set_kappa(const double value) { kappa_ = value; }

    [[nodiscard]] double get_alpha() const { return alpha_; }
    void set_alpha(const double value) { alpha_ = value; }

    [[nodiscard]] double get_beta() const { return beta_; }
    void set_beta(const double value) { beta_ = value; }

    [[nodiscard]] double get_gamma() const { return gamma_; }
    void set_gamma(const double value) { gamma_ = value; }

  private:
    double kappa_ = 0.01093251;
    double alpha_ = 0.22268982;
    double beta_ = 0.14319618;
    double gamma_ = 0.83741484;
  };

  class MarshallSM {
  public:
    [[nodiscard]] double get_tau() const { return tau_; }
    void set_tau(const double value) { tau_ = value; }

    [[nodiscard]] double get_alpha() const { return alpha_; }
    void set_alpha(const double value) { alpha_ = value; }

    [[nodiscard]] double get_log_rho() const { return log_rho_; }
    void set_log_rho(const double value) { log_rho_ = value; }

  private:
    double tau_ = 1.342;
    double alpha_ = 1.27;
    double log_rho_ = 0.54;
  };

  class BurkinaFasoSM {
  public:
    [[nodiscard]] double get_tau() const { return tau_; }
    void set_tau(const double value) { tau_ = value; }

    [[nodiscard]] double get_alpha() const { return alpha_; }
    void set_alpha(const double value) { alpha_ = value; }

    [[nodiscard]] double get_log_rho() const { return log_rho_; }
    void set_log_rho(const double value) { log_rho_ = value; }

    [[nodiscard]] double get_capital() const { return capital_; }
    void set_capital(const double value) { capital_ = value; }

    [[nodiscard]] double get_penalty() const { return penalty_; }
    void set_penalty(const double value) { penalty_ = value; }

  private:
    double tau_ = 1.342;
    double alpha_ = 1.27;
    double log_rho_ = 0.25;
    double capital_ = 14;
    double penalty_ = 12;
  };

  class SpatialModelSettings {
  public:
    // Getters and Setters
    [[nodiscard]] const std::string &get_name() const { return name_; }
    void set_name(const std::string &value) { name_ = value; }

    [[nodiscard]] const BarabasiSM &get_barabasi_sm() const { return barabasi_sm_; }
    void set_barabasi_sm(const BarabasiSM &value) { barabasi_sm_ = value; }

    [[nodiscard]] const WesolowskiSM &get_wesolowski_sm() const { return wesolowski_sm_; }
    void set_wesolowski_sm(const WesolowskiSM &value) { wesolowski_sm_ = value; }

    [[nodiscard]] const MarshallSM &get_marshall_sm() const { return marshall_sm_; }
    void set_marshall_sm(const MarshallSM &value) { marshall_sm_ = value; }

    [[nodiscard]] const BurkinaFasoSM &get_burkina_faso_sm() const { return burkina_faso_sm_; }
    void set_burkina_faso_sm(const BurkinaFasoSM &value) { burkina_faso_sm_ = value; }

    [[nodiscard]] const WesolowskiSurfaceSM &get_wesolowski_surface_sm() const {
      return wesolowski_surface_sm_;
    }
    void set_wesolowski_surface_sm(const WesolowskiSurfaceSM &value) {
      wesolowski_surface_sm_ = value;
    }

  private:
    std::string name_;
    BarabasiSM barabasi_sm_;
    WesolowskiSM wesolowski_sm_;
    WesolowskiSurfaceSM wesolowski_surface_sm_;
    MarshallSM marshall_sm_;
    BurkinaFasoSM burkina_faso_sm_;
  };

  class MovingLevelDistributionExponential {
  public:
    // Getters and Setters
    [[nodiscard]] double get_mean() const { return mean_; }
    void set_mean(const double value) { mean_ = value; }

    [[nodiscard]] double get_sd() const { return sd_; }
    void set_sd(const double value) { sd_ = value; }

    [[nodiscard]] double get_scale() const { return scale_; }
    void set_scale(const double value) { scale_ = value; }

  private:
    double scale_ = 0.17;
    double mean_ = 0.0;
    double sd_ = 0.0;
  };

  class LengthOfStay {
  public:
    // Getters and Setters
    [[nodiscard]] double get_mean() const { return mean_; }
    void set_mean(const double value) { mean_ = value; }

    [[nodiscard]] double get_sd() const { return sd_; }
    void set_sd(const double value) { sd_ = value; }

  private:
    double mean_ = 5;
    double sd_ = 10;
  };

  class MovingLevelDistributionGamma {
  public:
    // Getters and Setters
    [[nodiscard]] double get_mean() const { return mean_; }
    void set_mean(const double value) { mean_ = value; }

    [[nodiscard]] double get_sd() const { return sd_; }
    void set_sd(const double value) { sd_ = value; }

  private:
    double mean_ = 5;
    double sd_ = 10;
  };

  class MovingLevelDistribution {
  public:
    // Getters and Setters
    [[nodiscard]] const std::string &get_distribution() const { return distribution_; }
    void set_distribution(const std::string &value) { distribution_ = value; }

    [[nodiscard]] const MovingLevelDistributionGamma &get_gamma() const { return gamma_; }
    void set_gamma(const MovingLevelDistributionGamma &value) { gamma_ = value; }

    [[nodiscard]] const MovingLevelDistributionExponential &get_exponential() const {
      return exponential_;
    }
    void set_exponential(const MovingLevelDistributionExponential &value) { exponential_ = value; }

  private:
    std::string distribution_;
    MovingLevelDistributionGamma gamma_;
    MovingLevelDistributionExponential exponential_;
  };

  class CirculationInfo {
  public:
    // Getters and Setters
    [[nodiscard]] int get_max_relative_moving_value() const { return max_relative_moving_value_; }
    void set_max_relative_moving_value(const int value) {
      if (value < 0) throw std::invalid_argument("max_relative_moving_value must be non-negative");
      max_relative_moving_value_ = value;
    }

    [[nodiscard]] int get_number_of_moving_levels() const { return number_of_moving_levels_; }
    void set_number_of_moving_levels(const int value) {
      if (value >= 2) {
        number_of_moving_levels_ = value;
      } else {
        throw std::runtime_error(
            "number_of_moving_levels must be at least 2 (got " + std::to_string(value) + ")");
      }
    }

    [[nodiscard]] const MovingLevelDistribution &get_moving_level_distribution() const {
      return moving_level_distribution_;
    }
    void set_moving_level_distribution(const MovingLevelDistribution &value) {
      moving_level_distribution_ = value;
    }

    [[nodiscard]] double get_circulation_percent() const { return circulation_percent_; }
    void set_circulation_percent(const double value) {
      if (value < 0 || value > 100)
        throw std::invalid_argument("circulation_percent must be between 0 and 100");
      circulation_percent_ = value;
    }

    [[nodiscard]] const LengthOfStay &get_length_of_stay() const { return length_of_stay_; }
    void set_length_of_stay(const LengthOfStay &value) { length_of_stay_ = value; }

    [[nodiscard]] double get_relative_probability_that_child_travels_compared_to_adult() const {
      return relative_probability_that_child_travels_compared_to_adult_;
    }

    void set_relative_probability_that_child_travels_compared_to_adult(const double value) {
      relative_probability_that_child_travels_compared_to_adult_ = value;
    }

    [[nodiscard]] double get_relative_probability_for_clinical_to_travel() const {
      return relative_probability_for_clinical_to_travel_;
    }

    void set_relative_probability_for_clinical_to_travel(const double value) {
      relative_probability_for_clinical_to_travel_ = value;
    }

  private:
    MovingLevelDistribution moving_level_distribution_;
    MovingLevelDistributionGamma moving_level_distribution_gamma_;
    LengthOfStay length_of_stay_;

    int max_relative_moving_value_ = 35;
    int number_of_moving_levels_ = 100;
    double circulation_percent_ = 0.00336;

    double relative_probability_that_child_travels_compared_to_adult_ = 1.0;
    double relative_probability_for_clinical_to_travel_ = 1.0;
  };

  // Getters and Setters
  [[nodiscard]] const SpatialModelSettings &get_spatial_model_settings() const {
    return spatial_model_settings_;
  }
  void set_spatial_model_settings(const SpatialModelSettings &value) {
    spatial_model_settings_ = value;
  }

  [[nodiscard]] Spatial::SpatialModel* get_spatial_model() const { return spatial_model_.get(); }

  [[nodiscard]] const CirculationInfo &get_circulation_info() const { return circulation_info_; }
  void set_circulation_info(const CirculationInfo &value) { circulation_info_ = value; }

  [[nodiscard]] const std::vector<double> &get_v_moving_level_density() const {
    return v_moving_level_density_;
  }
  void set_v_moving_level_density(const std::vector<double> &value) {
    v_moving_level_density_ = value;
  }

  [[nodiscard]] const std::vector<double> &get_v_moving_level_value() const {
    return v_moving_level_value_;
  }
  void set_v_moving_level_value(const std::vector<double> &value) { v_moving_level_value_ = value; }

  [[nodiscard]] double get_length_of_stay_theta() const { return length_of_stay_theta_; }
  void set_length_of_stay_theta(const double value) { length_of_stay_theta_ = value; }

  [[nodiscard]] double get_length_of_stay_k() const { return length_of_stay_k_; }
  void set_length_of_stay_k(const double value) { length_of_stay_k_ = value; }

  // [[nodiscard]] const MultinomialDistributionGenerator &get_moving_level_generator() const {
  //   return moving_level_generator_;
  // }

  [[nodiscard]] MultinomialDistributionGenerator &get_moving_level_generator() {
    return moving_level_generator_;
  }

  void process_config() override {}

  void process_config_using_spatial_settings(
      const std::vector<std::vector<double>> &spatial_distance_matrix,
      const size_t number_of_locations) {
    spdlog::info("Processing MovementSettings");
    if (spatial_model_settings_.get_name() == "Barabasi") {
      spdlog::info("Processing BarabasiSM");
      spatial_model_ = std::make_unique<Spatial::BarabasiSM>(
          spatial_model_settings_.get_barabasi_sm().get_r_g_0(),
          spatial_model_settings_.get_barabasi_sm().get_beta_r(),
          spatial_model_settings_.get_barabasi_sm().get_kappa());
    } else if (spatial_model_settings_.get_name() == "Wesolowski") {
      spdlog::info("Processing WesolowskiSM");
      spatial_model_ = std::make_unique<Spatial::WesolowskiSM>(
          spatial_model_settings_.get_wesolowski_sm().get_kappa(),
          spatial_model_settings_.get_wesolowski_sm().get_alpha(),
          spatial_model_settings_.get_wesolowski_sm().get_beta(),
          spatial_model_settings_.get_wesolowski_sm().get_gamma());
    } else if (spatial_model_settings_.get_name() == "WesolowskiSurface") {
      spdlog::info("Processing WesolowskiSurfaceSM");
      spatial_model_ = std::make_unique<Spatial::WesolowskiSurfaceSM>(
          spatial_model_settings_.get_wesolowski_surface_sm().get_kappa(),
          spatial_model_settings_.get_wesolowski_surface_sm().get_alpha(),
          spatial_model_settings_.get_wesolowski_surface_sm().get_beta(),
          spatial_model_settings_.get_wesolowski_surface_sm().get_gamma(), number_of_locations);
    } else if (spatial_model_settings_.get_name() == "Marshall") {
      spdlog::info("Processing MarshallSM");
      spatial_model_ = std::make_unique<Spatial::MarshallSM>(
          spatial_model_settings_.get_marshall_sm().get_tau(),
          spatial_model_settings_.get_marshall_sm().get_alpha(),
          spatial_model_settings_.get_marshall_sm().get_log_rho(), number_of_locations,
          spatial_distance_matrix);
    } else if (spatial_model_settings_.get_name() == "BurkinaFaso") {
      spdlog::info("Processing BurkinaFasoSM");
      spatial_model_ = std::make_unique<Spatial::BurkinaFasoSM>(
          spatial_model_settings_.get_burkina_faso_sm().get_tau(),
          spatial_model_settings_.get_burkina_faso_sm().get_alpha(),
          spatial_model_settings_.get_burkina_faso_sm().get_log_rho(),
          spatial_model_settings_.get_burkina_faso_sm().get_capital(),
          spatial_model_settings_.get_burkina_faso_sm().get_penalty(), number_of_locations,
          spatial_distance_matrix);
    }
    // Circulation Info
    // calculate density and level value here
    const auto var = get_circulation_info().get_moving_level_distribution().get_gamma().get_sd()
                     * get_circulation_info().get_moving_level_distribution().get_gamma().get_sd();
    const auto b_value =
        var
        / (get_circulation_info().get_moving_level_distribution().get_gamma().get_mean()
           - 1);  // theta
    const auto a_value =
        (get_circulation_info().get_moving_level_distribution().get_gamma().get_mean() - 1)
        / b_value;  // k

    v_moving_level_density_.clear();
    v_moving_level_value_.clear();

    const auto max =
        get_circulation_info().get_max_relative_moving_value() - 1;  // maxRelativeBiting -1
    const auto number_of_level = get_circulation_info().get_number_of_moving_levels();
    if (number_of_level < 2) {
      throw std::runtime_error(
          "MovementSettings: number_of_moving_levels must be >= 2 "
          "to compute relative moving steps");
    }
    const auto step = max / static_cast<double>(number_of_level - 1);

    auto level_index = 0;
    double old_p = 0;
    double sum = 0;
    for (double i = 0; i <= max + 0.0001; i += step) {
      const auto prob = gsl_cdf_gamma_P(i + step, a_value, b_value);
      double value = 0;
      value = level_index == 0 ? prob : prob - old_p;
      v_moving_level_density_.push_back(value);
      old_p = prob;
      v_moving_level_value_.push_back(i + 1);
      sum += value;
      level_index++;
    }

    // normalized
    double prob_sum = 0;
    for (auto &density : v_moving_level_density_) {
      density = density + (1 - sum) / static_cast<double>(v_moving_level_density_.size());
      prob_sum += density;
    }

    assert(static_cast<unsigned>(get_circulation_info().get_number_of_moving_levels())
           == v_moving_level_density_.size());
    assert(static_cast<unsigned>(get_circulation_info().get_number_of_moving_levels())
           == v_moving_level_value_.size());
    assert(fabs(prob_sum - 1) < 0.0001);

    const auto stay_variance = get_circulation_info().get_length_of_stay().get_sd()
                               * get_circulation_info().get_length_of_stay().get_sd();
    const auto k_value =
        stay_variance / get_circulation_info().get_length_of_stay().get_mean();           // k
    const auto theta = get_circulation_info().get_length_of_stay().get_mean() / k_value;  // theta

    length_of_stay_theta_ = theta;
    length_of_stay_k_ = k_value;

    moving_level_generator_.level_density = v_moving_level_density_;
  }

private:
  SpatialModelSettings spatial_model_settings_;
  std::unique_ptr<Spatial::SpatialModel> spatial_model_{nullptr};
  CirculationInfo circulation_info_;
  DoubleVector v_moving_level_value_;
  DoubleVector v_moving_level_density_;
  MultinomialDistributionGenerator moving_level_generator_;
  double length_of_stay_ = 0.0;
  double length_of_stay_mean_ = 0.0;
  double length_of_stay_sd_ = 0.0;
  double length_of_stay_theta_ = 0.0;
  double length_of_stay_k_ = 0.0;
};

namespace YAML {
// BarabasiSM YAML conversion
template <>
struct convert<MovementSettings::BarabasiSM> {
  static Node encode(const MovementSettings::BarabasiSM &rhs) {
    Node node;
    node["r_g_0"] = rhs.get_r_g_0();
    node["beta_r"] = rhs.get_beta_r();
    node["kappa"] = rhs.get_kappa();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::BarabasiSM &rhs) {
    if (!node["r_g_0"] || !node["beta_r"] || !node["kappa"])
      throw std::runtime_error("Missing fields in BarabasiSettings");

    rhs.set_r_g_0(node["r_g_0"].as<double>());
    rhs.set_beta_r(node["beta_r"].as<double>());
    rhs.set_kappa(node["kappa"].as<int>());
    return true;
  }
};

// WesolowskiSM YAML conversion
template <>
struct convert<MovementSettings::WesolowskiSM> {
  static Node encode(const MovementSettings::WesolowskiSM &rhs) {
    Node node;
    node["kappa"] = rhs.get_kappa();
    node["alpha"] = rhs.get_alpha();
    node["beta"] = rhs.get_beta();
    node["gamma"] = rhs.get_gamma();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::WesolowskiSM &rhs) {
    if (!node["kappa"] || !node["alpha"] || !node["beta"] || !node["gamma"])
      throw std::runtime_error("Missing fields in WesolowskiSM");

    rhs.set_kappa(node["kappa"].as<double>());
    rhs.set_alpha(node["alpha"].as<double>());
    rhs.set_beta(node["beta"].as<double>());
    rhs.set_gamma(node["gamma"].as<double>());
    return true;
  }
};

// WesolowskiSurfaceSM YAML conversion
template <>
struct convert<MovementSettings::WesolowskiSurfaceSM> {
  static Node encode(const MovementSettings::WesolowskiSurfaceSM &rhs) {
    Node node;
    node["kappa"] = rhs.get_kappa();
    node["alpha"] = rhs.get_alpha();
    node["beta"] = rhs.get_beta();
    node["gamma"] = rhs.get_gamma();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::WesolowskiSurfaceSM &rhs) {
    if (!node["kappa"] || !node["alpha"] || !node["beta"] || !node["gamma"])
      throw std::runtime_error("Missing fields in WesolowskiSurfaceSM");

    rhs.set_kappa(node["kappa"].as<double>());
    rhs.set_alpha(node["alpha"].as<double>());
    rhs.set_beta(node["beta"].as<double>());
    rhs.set_gamma(node["gamma"].as<double>());
    return true;
  }
};

// MarshallSM YAML conversion
template <>
struct convert<MovementSettings::MarshallSM> {
  static Node encode(const MovementSettings::MarshallSM &rhs) {
    Node node;
    node["tau"] = rhs.get_tau();
    node["alpha"] = rhs.get_alpha();
    node["log_rho"] = rhs.get_log_rho();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::MarshallSM &rhs) {
    if (!node["tau"] || !node["alpha"] || !node["log_rho"])
      throw std::runtime_error("Missing fields in MarshallSM");

    rhs.set_tau(node["tau"].as<double>());
    rhs.set_alpha(node["alpha"].as<double>());
    rhs.set_log_rho(node["log_rho"].as<double>());
    return true;
  }
};

// BurkinaFasoSM YAML conversion
template <>
struct convert<MovementSettings::BurkinaFasoSM> {
  static Node encode(const MovementSettings::BurkinaFasoSM &rhs) {
    Node node;
    node["tau"] = rhs.get_tau();
    node["alpha"] = rhs.get_alpha();
    node["log_rho"] = rhs.get_log_rho();
    node["capital"] = rhs.get_capital();
    node["penalty"] = rhs.get_penalty();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::BurkinaFasoSM &rhs) {
    if (!node["tau"] || !node["alpha"] || !node["log_rho"] || !node["capital"] || !node["penalty"])
      throw std::runtime_error("Missing fields in BurkinaFasoSM");

    rhs.set_tau(node["tau"].as<double>());
    rhs.set_alpha(node["alpha"].as<double>());
    rhs.set_log_rho(node["log_rho"].as<double>());
    rhs.set_capital(node["capital"].as<double>());
    rhs.set_penalty(node["penalty"].as<double>());
    return true;
  }
};

// SpatialModel YAML conversion
template <>
struct convert<MovementSettings::SpatialModelSettings> {
  static Node encode(const MovementSettings::SpatialModelSettings &rhs) {
    Node node;
    node["name"] = rhs.get_name();
    node["Barabasi"] = rhs.get_barabasi_sm();
    node["Wesolowski"] = rhs.get_wesolowski_sm();
    node["Marshall"] = rhs.get_marshall_sm();
    node["BurkinaFaso"] = rhs.get_burkina_faso_sm();
    node["WesolowskiSurface"] = rhs.get_wesolowski_surface_sm();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::SpatialModelSettings &rhs) {
    if (!node["name"]) { throw std::runtime_error("Missing fields in SpatialModel"); }
    rhs.set_name(node["name"].as<std::string>());

    if (node["Barabasi"]) {
      rhs.set_barabasi_sm(node["Barabasi"].as<MovementSettings::BarabasiSM>());
    }
    if (node["Wesolowski"]) {
      rhs.set_wesolowski_sm(node["Wesolowski"].as<MovementSettings::WesolowskiSM>());
    }
    if (node["Marshall"]) {
      rhs.set_marshall_sm(node["Marshall"].as<MovementSettings::MarshallSM>());
    }
    if (node["BurkinaFaso"]) {
      rhs.set_burkina_faso_sm(node["BurkinaFaso"].as<MovementSettings::BurkinaFasoSM>());
    }
    if (node["WesolowskiSurface"]) {
      rhs.set_wesolowski_surface_sm(
          node["WesolowskiSurface"].as<MovementSettings::WesolowskiSurfaceSM>());
    }
    return true;
  }
};

// ExponentialDistribution YAML conversion
template <>
struct convert<MovementSettings::MovingLevelDistributionExponential> {
  static Node encode(const MovementSettings::MovingLevelDistributionExponential &rhs) {
    Node node;
    node["scale"] = rhs.get_scale();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::MovingLevelDistributionExponential &rhs) {
    if (!node["scale"]) throw std::runtime_error("Missing fields in ExponentialDistribution");

    rhs.set_scale(node["scale"].as<double>());
    return true;
  }
};

// GammaDistribution YAML conversion
template <>
struct convert<MovementSettings::MovingLevelDistributionGamma> {
  static Node encode(const MovementSettings::MovingLevelDistributionGamma &rhs) {
    Node node;
    node["mean"] = rhs.get_mean();
    node["sd"] = rhs.get_sd();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::MovingLevelDistributionGamma &rhs) {
    if (!node["mean"] || !node["sd"])
      throw std::runtime_error("Missing fields in GammaDistribution");

    rhs.set_mean(node["mean"].as<double>());
    rhs.set_sd(node["sd"].as<double>());
    return true;
  }
};

// LengthOfStay YAML conversion
template <>
struct convert<MovementSettings::LengthOfStay> {
  static Node encode(const MovementSettings::LengthOfStay &rhs) {
    Node node;
    node["mean"] = rhs.get_mean();
    node["sd"] = rhs.get_sd();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::LengthOfStay &rhs) {
    if (!node["mean"] || !node["sd"]) throw std::runtime_error("Missing fields in LengthOfStay");

    rhs.set_mean(node["mean"].as<double>());
    rhs.set_sd(node["sd"].as<double>());
    return true;
  }
};

// MovingLevelDistribution YAML conversion
template <>
struct convert<MovementSettings::MovingLevelDistribution> {
  static Node encode(const MovementSettings::MovingLevelDistribution &rhs) {
    Node node;
    node["distribution"] = rhs.get_distribution();
    node["Gamma"] = rhs.get_gamma();
    node["Exponential"] = rhs.get_exponential();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::MovingLevelDistribution &rhs) {
    if (!node["distribution"] || !node["Gamma"] || !node["Exponential"])
      throw std::runtime_error("Missing fields in MovingLevelDistribution");

    rhs.set_distribution(node["distribution"].as<std::string>());
    rhs.set_gamma(node["Gamma"].as<MovementSettings::MovingLevelDistributionGamma>());
    rhs.set_exponential(
        node["Exponential"].as<MovementSettings::MovingLevelDistributionExponential>());
    return true;
  }
};

// CirculationInfo YAML conversion
template <>
struct convert<MovementSettings::CirculationInfo> {
  static Node encode(const MovementSettings::CirculationInfo &rhs) {
    Node node;
    node["max_relative_moving_value"] = rhs.get_max_relative_moving_value();
    node["number_of_moving_levels"] = rhs.get_number_of_moving_levels();
    node["moving_level_distribution"] = rhs.get_moving_level_distribution();
    node["circulation_percent"] = rhs.get_circulation_percent();
    node["length_of_stay"] = rhs.get_length_of_stay();
    node["relative_probability_that_child_travels_compared_to_adult"] =
        rhs.get_relative_probability_that_child_travels_compared_to_adult();
    node["relative_probability_for_clinical_to_travel"] =
        rhs.get_relative_probability_for_clinical_to_travel();
    return node;
  }

  static bool decode(const Node &node, MovementSettings::CirculationInfo &rhs) {
    if (!node["max_relative_moving_value"] || !node["number_of_moving_levels"]
        || !node["moving_level_distribution"] || !node["circulation_percent"]
        || !node["length_of_stay"])
      throw std::runtime_error("Missing fields in CirculationInfo");

    rhs.set_max_relative_moving_value(node["max_relative_moving_value"].as<int>());
    rhs.set_number_of_moving_levels(node["number_of_moving_levels"].as<int>());
    rhs.set_moving_level_distribution(
        node["moving_level_distribution"].as<MovementSettings::MovingLevelDistribution>());
    rhs.set_circulation_percent(node["circulation_percent"].as<double>());
    rhs.set_length_of_stay(node["length_of_stay"].as<MovementSettings::LengthOfStay>());
    if (node["relative_probability_that_child_travels_compared_to_adult"]) {
      rhs.set_relative_probability_that_child_travels_compared_to_adult(
          node["relative_probability_that_child_travels_compared_to_adult"].as<double>());
    } else {
      spdlog::info(
          "Relative probability that child travels compared to adult is "
          "not set in input file, defaulting to 1.0");
      rhs.set_relative_probability_that_child_travels_compared_to_adult(1.0);
    }
    if (node["relative_probability_for_clinical_to_travel"]) {
      rhs.set_relative_probability_for_clinical_to_travel(
          node["relative_probability_that_child_travels_compared_to_adult"].as<double>());
    } else {
      // log warning
      spdlog::info(
          "Relative probability for a clinical case to travel is "
          "not set in input file, defaulting to 1.0");
      rhs.set_relative_probability_for_clinical_to_travel(1.0);
    }
    return true;
  }
};

// MovementSettings YAML conversion
template <>
struct convert<MovementSettings> {
  static Node encode(const MovementSettings &rhs) {
    Node node;
    node["spatial_model"] = rhs.get_spatial_model_settings();
    node["circulation_info"] = rhs.get_circulation_info();
    return node;
  }

  static bool decode(const Node &node, MovementSettings &rhs) {
    if (!node["spatial_model"] || !node["circulation_info"])
      throw std::runtime_error("Missing fields in MovementSettings");

    rhs.set_spatial_model_settings(
        node["spatial_model"].as<MovementSettings::SpatialModelSettings>());
    rhs.set_circulation_info(node["circulation_info"].as<MovementSettings::CirculationInfo>());
    return true;
  }
};
}  // namespace YAML
