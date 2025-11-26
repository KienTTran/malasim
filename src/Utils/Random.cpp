/*
 * Random.cpp
 *
 * This file implements the random number class for use by the simulation.
 */
#include "Random.h"

#include <fmt/format.h>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_randist.h>

#include <cmath>  // Ensure cmath is included for std::round
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

using utils::Random;

// Constructor
Random::Random(gsl_rng* rng, uint64_t seed) : seed_(seed) {
  if (rng != nullptr) {
    // If an external RNG is provided, take ownership using the unique_ptr
    rng_.reset(rng);
  } else {
    // Initialize with default seed
    initialize(seed);
  }
}

void Random::initialize(uint64_t initial_seed) {
  // Select the Mersenne Twister 19937 generator
  const gsl_rng_type* rng_type = gsl_rng_mt19937;

  // Allocate the GSL RNG and assign it to the unique_ptr
  rng_.reset(gsl_rng_alloc(rng_type));
  if (!rng_) { throw std::runtime_error("Failed to allocate GSL random number generator."); }

  // Use std::random_device to generate a random seed if seed is -1 (UINT64_MAX)
  std::random_device rd;
  seed_ = (initial_seed == static_cast<uint64_t>(-1)) ? rd() : initial_seed;

  // Set the seed for the GSL RNG
  gsl_rng_set(rng_.get(), seed_);
}

// Getter for seed
uint64_t Random::get_seed() const noexcept { return seed_; }

// Setter for seed
void Random::set_seed(uint64_t new_seed) {
  seed_ = new_seed;

  // If RNG hasn't been allocated yet, do it once here
  if (!rng_) {
    initialize(seed_);
    return;
  }

  // Otherwise, just reseed the existing generator
  gsl_rng_set(rng_.get(), seed_);
}

// Generates a Poisson-distributed random number
int Random::random_poisson(double poisson_mean) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  return static_cast<int>(gsl_ran_poisson(rng_.get(), poisson_mean));
}

// Generates a uniform random integer in [0, range)
uint64_t Random::random_uniform(uint64_t upper_bound) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  return gsl_rng_uniform_int(rng_.get(), upper_bound);
}

// Generates a uniform random double in [0, 1)
double Random::random_uniform() {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  return gsl_rng_uniform(rng_.get());
}

// Generates a Beta-distributed random double
double Random::random_beta(double alpha, double beta) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  if (alpha <= 0.0 || beta < 0) {
    throw std::invalid_argument("Parameters 'alpha' and 'beta' must be greater than 0.");
  }
  if (beta == 0.0) { return alpha; }
  return gsl_ran_beta(rng_.get(), alpha, beta);
}

// Generates a Gamma-distributed random double
double Random::random_gamma(double shape, double scale) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  if (shape <= 0.0 || scale < 0) {
    throw std::invalid_argument("Parameters 'shape' and 'scale' must be greater than 0.");
  }
  if (scale == 0) { return shape; }
  return gsl_ran_gamma(rng_.get(), shape, scale);
}

// Generates multinomially distributed random numbers
void Random::random_multinomial(std::size_t categories, unsigned trials,
                                const std::vector<double> &probabilities,
                                std::vector<unsigned> &results) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  if (categories == 0) {
    throw std::invalid_argument("Parameter 'categories' must be greater than 0.");
  }
  if (probabilities.size() != categories || results.size() != categories) {
    throw std::invalid_argument("Size of 'probabilities' and 'results' must match 'categories'.");
  }
  gsl_ran_multinomial(rng_.get(), static_cast<unsigned int>(categories), trials,
                      probabilities.data(), results.data());
}

// Generates a binomially distributed random integer
unsigned int Random::random_binomial(double probability, unsigned int trials) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  if (probability < 0.0 || probability > 1.0) {
    throw std::invalid_argument("Parameter 'probability' must be in [0, 1].");
  }
  return gsl_ran_binomial(rng_.get(), probability, trials);
}

// Computes the CDF of the Gamma distribution
double Random::cdf_gamma_distribution(double value, double alpha, double beta) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  if (alpha <= 0.0 || beta < 0) {
    throw std::invalid_argument("Parameters 'alpha' and 'beta' must be greater than 0.");
  }
  if (beta == 0) { return 1.0; }
  return gsl_cdf_gamma_P(value, alpha, beta);
}

// Computes the inverse CDF (quantile function) of the Gamma distribution
double Random::cdf_gamma_distribution_inverse(double probability, double alpha, double beta) {
  if (!rng_) { throw std::runtime_error("Random number generator not initialized."); }
  if (probability < 0.0 || probability > 1.0) {
    throw std::invalid_argument("Parameter 'probability' must be in [0, 1].");
  }
  return gsl_cdf_gamma_Pinv(probability, alpha, beta);
}

/**
 * @brief Approximates the cumulative distribution function (CDF) of the standard normal
 * distribution.
 *
 * This function uses a polynomial approximation (Abramowitz and Stegun formula 26.2.17)
 * to estimate the probability P(Z <= x) where Z is a standard normal random variable.
 * The approximation is reasonably accurate for values of x within a certain range (e.g., -6 to +6).
 *
 * @param x The value at which to evaluate the standard normal CDF.
 * @return The approximate probability P(Z <= x).
 */
double approx_norm_cdf(double xx) {
  // Constants
  const double b1 = 0.319381530;
  const double b2 = -0.356563782;
  const double b3 = 1.781477937;
  const double b4 = -1.821255978;
  const double b5 = 1.330274429;
  const double pp = 0.2316419;
  const double cc = 0.39894228;

  if (xx > 6.0) return 1.0;
  if (xx < -6.0) return 0.0;

  double tt = 1.0 / (1.0 + pp * std::fabs(xx));
  double poly = (((((b5 * tt + b4) * tt + b3) * tt + b2) * tt + b1) * tt);
  double approx = 1.0 - (cc * std::exp(-xx * xx / 2.0) * poly);
  return (xx < 0.0) ? 1.0 - approx : approx;
}

// Computes the CDF of the standard normal distribution
double Random::cdf_standard_normal_distribution(double value) {
  // No RNG needed for a pure CDF
  return gsl_cdf_ugaussian_P(value);
  // or: return approx_norm_cdf(value);
}

double Random::random_flat(double from, double to) { return gsl_ran_flat(rng_.get(), from, to); }

// Implementation for random_normal (double)
double Random::random_normal_double(double mean, double standard_deviation) {
  return random_normal<double>(mean, standard_deviation);
}

// Implementation for random_normal (int)
int Random::random_normal_int(int mean, double standard_deviation) {
  return random_normal<int>(mean, standard_deviation);
}
