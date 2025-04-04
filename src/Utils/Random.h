// Random.h

#ifndef RANDOM_H
#define RANDOM_H

#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <uuid.h>
#include <spdlog/spdlog.h>

#include <vector>

namespace utils {
/**
 * @class Random
 * @brief Encapsulates random number generation functionalities using GSL.
 *
 * The Random class provides various methods to generate random numbers
 * following different distributions. It manages a GSL random number generator
 * (RNG) instance, ensuring proper resource management and offering a clean
 * interface for simulations.
 */
class Random {
public:
  // Delete copy constructor and copy assignment operator
  Random(const Random &) = delete;
  Random &operator=(const Random &) = delete;

  // Delete move constructor and move assignment operator
  Random(Random &&) = delete;
  Random &operator=(Random &&) = delete;

  /**
   * @brief Constructs a Random number generator.
   *
   * If an external GSL RNG is provided, the class takes ownership of it.
   * Otherwise, it initializes a new RNG with a default or specified seed.
   *
   * @param external_rng Pointer to an external GSL RNG. Defaults to nullptr.
   */
  explicit Random(gsl_rng* external_rng = nullptr, uint64_t seed = -1);

  /**
   * @brief Destructor.
   *
   * Ensures proper resource deallocation of the GSL RNG.
   */
  virtual ~Random() = default;

  /**
   * @brief Retrieves the current RNG seed.
   *
   * @return uint64_t The seed value.
   */
  [[nodiscard]] uint64_t get_seed() const noexcept;

  /**
   * @brief Sets a new seed for the RNG.
   *
   * @param new_seed The seed value to set.
   */
  void set_seed(uint64_t new_seed);

  // Random number generation methods

  /**
   * @brief Generates a Poisson-distributed random number.
   *
   * @param mean The mean of the Poisson distribution.
   * @return int A random number following the Poisson distribution.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  int random_poisson(double mean);

  /**
   * @brief Generates a uniform random integer in [0, upper_bound).
   *
   * @param upper_bound The exclusive upper bound for the random number.
   * @return uint64_t A uniformly distributed random integer.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  virtual uint64_t random_uniform(uint64_t upper_bound);

  /**
   * @brief Generates a uniform random double in [0, 1).
   *
   * @return double A uniformly distributed random double.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  virtual double random_uniform();

  /**
   * @brief Generates a uniformly distributed random number within a specified
   * range.
   *
   * This function returns a random number of type `T` that is uniformly
   * distributed between the specified `from` and `to` values. The range is
   * inclusive of `from` and exclusive of `to`. It supports both integral and
   * floating-point types.
   *
   * @tparam T The numeric type of the random number to generate. Must be an
   * arithmetic type.
   *
   * @param from The lower bound of the range. Must be less than `to`.
   * @param to The upper bound of the range. Must be greater than `from`.
   *
   * @return A random number of type `T` uniformly distributed in [from, to).
   *
   * @throws std::runtime_error If the random number generator is not
   * initialized.
   * @throws std::invalid_argument If `from` is not less than `to`.
   *
   * @note Ensure that the random number generator (`rng_`) is properly
   * initialized before invoking this function.
   */
  template <typename T>
  T random_uniform(T from, T to) {
    static_assert(std::is_arithmetic_v<T>,
                  "Template parameter T must be numeric.");

    if (!rng_) {
      throw std::runtime_error("Random number generator not initialized.");
    }

    if (from >= to) {
      throw std::invalid_argument("Parameter 'from' must be less than 'to'.");
    }

    if constexpr (std::is_integral_v<T>) {
      // gsl_rng_uniform_int takes an unsigned long as upper bound
      // Ensure that (to - from) does not exceed the maximum value of unsigned
      // long
      auto range = static_cast<uint64_t>(to - from);
      // Note: gsl_rng_uniform_int handles range values up to the maximum of
      // unsigned long
      uint64_t value = gsl_rng_uniform_int(rng_.get(), range);
      return static_cast<T>(from + value);
    } else {
      double value = gsl_ran_flat(rng_.get(), static_cast<double>(from),
                                  static_cast<double>(to));
      return static_cast<T>(value);
    }
  }

  /**
   * @brief Generates a truncated normally distributed random number within a
   * specified number of standard deviations.
   *
   * The function repeatedly generates normally distributed random numbers until
   * one falls within the range [mean - truncation_limit * standard_deviation,
   * mean + truncation_limit * standard_deviation]. To prevent potential
   * infinite loops, a maximum number of attempts is enforced.
   *
   * @tparam T The return type, typically `double` or `int`.
   * @param mean The mean of the normal distribution.
   * @param standard_deviation The standard deviation of the normal
   * distribution.
   * @param truncation_limit The number of standard deviations to truncate
   * (default is 3.0).
   * @param max_attempts The maximum number of attempts to generate a valid
   * truncated value (default is 1000).
   * @return T A truncated normally distributed random number.
   *
   * @throws std::runtime_error If RNG is not initialized or if a valid
   * truncated value cannot be generated within the maximum attempts.
   */
  template <typename T>
  T random_normal_truncated(T mean, double standard_deviation,
                            double truncation_limit = 3.0,  // NOLINT
                            int max_attempts = 1000) {
    static_assert(std::is_arithmetic_v<T>,
                  "Template parameter T must be numeric.");
    if (!rng_) {
      throw std::runtime_error("Random number generator not initialized.");
    }
    double value = gsl_ran_gaussian(rng_.get(), standard_deviation);
    int attempts = 0;
    while (std::abs(value) > truncation_limit * standard_deviation) {
      if (attempts++ >= max_attempts) {
        throw std::runtime_error(
            "Failed to generate a truncated normal value within the maximum "
            "number of attempts.");
      }
      value = gsl_ran_gaussian(rng_.get(), standard_deviation);
    }
    if constexpr (std::is_integral_v<T>) {
      return static_cast<T>(mean + std::round(value));
    } else {
      return static_cast<T>(mean + value);
    }
  }

  /**
   * @brief Generates a Beta-distributed random double.
   *
   * @param alpha The alpha parameter of the Beta distribution.
   * @param beta The beta parameter of the Beta distribution.
   * @return double A random number following the Beta distribution.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  double random_beta(double alpha, double beta);

  /**
   * @brief Generates a Gamma-distributed random double.
   *
   * @param shape The shape parameter of the Gamma distribution.
   * @param scale The scale parameter of the Gamma distribution.
   * @return double A random number following the Gamma distribution.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  double random_gamma(double shape, double scale);


  virtual double random_flat(double from, double to);

  /**
   * @brief Computes the CDF of the Gamma distribution.
   *
   * @param value The value at which to evaluate the CDF.
   * @param shape The shape parameter of the Gamma distribution.
   * @param scale The scale parameter of the Gamma distribution.
   * @return double The CDF value.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  double cdf_gamma_distribution(double value, double alpha, double beta);

  /**
   * @brief Computes the inverse CDF (quantile function) of the Gamma
   * distribution.
   *
   * @param probability The probability value for which to compute the quantile.
   * @param shape The shape parameter of the Gamma distribution.
   * @param scale The scale parameter of the Gamma distribution.
   * @return double The quantile corresponding to the given probability.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  double cdf_gamma_distribution_inverse(double probability, double alpha,
                                        double beta);

  /**
   * @brief Generates multinomial random variables.
   *
   * @param categories Number of categories.
   * @param trials Number of trials.
   * @param probabilities Vector of probabilities (size `categories`),
   * does not need to normalize (or sum can be different than 1).
   * @param results Vector where results will be stored (size `categories`).
   *
   * @throws std::invalid_argument If `categories` is zero.
   * @throws std::runtime_error If RNG is not initialized.
   */
  void random_multinomial(std::size_t categories, unsigned trials,
                          const std::vector<double> &probabilities,
                          std::vector<unsigned> &results);

  /**
   * @brief Computes the CDF of the standard normal distribution.
   *
   * @param value The value at which to evaluate the CDF.
   * @return double The CDF value.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  virtual double cdf_standard_normal_distribution(double value);

  /**
   * @brief Generates a binomially distributed random integer.
   *
   * @param probability The probability of success in each trial.
   * @param trials The number of trials.
   * @return int A binomially distributed random integer.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  unsigned int random_binomial(double probability, unsigned int trials);

  /**
   * @brief Shuffles the elements of a vector in place using the current random
   *number generator.
   *
   * This templated function randomizes the order of elements within the
   *provided `std::vector<T>` using GSL's `gsl_ran_shuffle` function. It ensures
   *that the shuffling process maintains the integrity of the data, meaning no
   *elements are lost or duplicated during the operation.
   *
   * @tparam T The type of elements stored in the vector. Can be any copyable
   *and movable type.
   *
   * @param vec A reference to the `std::vector<T>` whose elements are to be
   *shuffled. Must not be empty.
   *
   * @throws std::runtime_error If the random number generator (`rng_`) is not
   *initialized.
   * @throws std::invalid_argument If the provided vector `vec` is empty.
   *
   * @note
   * - The function requires that the random number generator (`rng_`) is
   *properly initialized.
   * - The randomness of the shuffle is dependent on the underlying RNG's
   *quality and seeding.
   *
   * @warning
   * - Ensure that the `Random` class instance has been initialized with a valid
   *RNG before calling this function.
   **/
  // NOTE:: return a new vector (functional style) or modify in place
  // (imperative style)
  template <typename T>
  void shuffle(std::vector<T> &vec) const {
    // Check if RNG is initialized
    if (!rng_) {
      throw std::runtime_error("Random number generator not initialized.");
    }

    // Check if the vector is empty
    if (vec.empty()) {
      throw std::invalid_argument("Vector must not be empty.");
    }

    // Shuffle using gsl_ran_shuffle
    gsl_ran_shuffle(rng_.get(), vec.data(), vec.size(), sizeof(T));
  }

    /**
     * @brief Generates a new UUID.
     *
     * This function uses the `uuid_random_generator` to produce a new universally
     * unique identifier (UUID). It can be used to generate unique IDs for various
     * purposes, such as object identifiers or session tokens.
     *
     * @return uuids::uuid A newly generated UUID.
     */
    uuids::uuid uuid() {
      std::random_device rd;
      std::mt19937 generator{rd()};
      return uuids::uuid_random_generator{generator}();
  }

private:
    uint64_t seed_;

  // Custom deleter for gsl_rng
  struct GslRngDeleter {
    void operator()(gsl_rng* rng) const {
      if (rng != nullptr) { gsl_rng_free(rng); }
    }
  };

  // Unique pointer managing gsl_rng with custom deleter
  std::unique_ptr<gsl_rng, GslRngDeleter> rng_;

  /**
   * @brief Initializes the GSL random number generator with a given seed.
   *
   * If the provided seed is zero, it uses `std::random_device` to generate a
   * random seed.
   *
   * @param initial_seed Seed value. Defaults to 0.
   *
   * @throws std::runtime_error If RNG allocation fails.
   */
  void initialize(uint64_t initial_seed = 0);

public:

  template <class T>
  [[nodiscard]] std::vector<T *> multinomial_sampling(int size, std::vector<double> &distribution,
                                                    std::vector<T *> &all_objects, bool is_shuffled,
                                                    double sum_distribution = -1);

  template <class T>
  [[nodiscard]] std::vector<T *> roulette_sampling(int number_of_samples, std::vector<double> &distribution,
                                                   std::vector<T *> &all_objects, bool is_shuffled,
                                                   double sum_distribution = -1);

  template <class T>
  [[nodiscard]] std::vector<std::tuple<T *, double>> roulette_sampling_tuple(int number_of_samples,
                                                                             std::vector<double> &distribution,
                                                                             std::vector<T *> &all_objects,
                                                                             bool is_shuffled,
                                                                             double sum_distribution = -1);

  /**
   * @brief Generates a normally distributed random number with given mean and
   * standard deviation.
   *
   * @param mean The mean of the normal distribution (integer).
   * @param standard_deviation The standard deviation of the normal
   * distribution.
   * @return A random integer following the normal distribution.
   *
   * @throws std::runtime_error If RNG is not initialized.
   * @throws std::invalid_argument If standard_deviation <= 0.
   */
  virtual int random_normal_int(int mean, double standard_deviation);

  /**
   * @brief Generates a normally distributed random number with given mean and
   * standard deviation.
   *
   * @param mean The mean of the normal distribution.
   * @param standard_deviation The standard deviation of the normal
   * distribution.
   * @return A random double following the normal distribution.
   *
   * @throws std::runtime_error If RNG is not initialized.
   * @throws std::invalid_argument If standard_deviation <= 0.
   */
  virtual double random_normal_double(double mean, double standard_deviation);

private:
 /**
   * @brief Generates a normally distributed random number with given mean and
   * standard deviation.
   *
   * @tparam T The return type, typically `double` or `int`.
   * @param mean The mean of the normal distribution.
   * @param standard_deviation The standard deviation of the normal
   * distribution.
   * @return T A random number following the normal distribution.
   *
   * @throws std::runtime_error If RNG is not initialized.
   */
  template <typename T>
  T random_normal(T mean, double standard_deviation) {
    static_assert(std::is_arithmetic_v<T>,
                  "Template parameter T must be numeric.");

    if (!rng_) {
      throw std::runtime_error("Random number generator not initialized.");
    }

    if (standard_deviation <= 0) {
      throw std::invalid_argument(
          "Parameters 'standard_deviation' must be greater than 0.");
    }
    double value = mean + gsl_ran_gaussian(rng_.get(), standard_deviation);

    if constexpr (std::is_integral_v<T>) {
      return static_cast<T>(std::round(value));
    } else {
      return static_cast<T>(value);
    }
  }
};
}  // namespace utils


template <class T>
std::vector<T *> utils::Random::multinomial_sampling(int size, std::vector<double> &distribution,
                                                     std::vector<T *> &all_objects, bool is_shuffled,
                                                     double sum_distribution) {
  std::vector<T *> samples(size, nullptr);
  if (sum_distribution == 0) {
    return samples;
  } else if (sum_distribution < 0) {
    auto found = std::find_if(distribution.begin(), distribution.end(), [](double d) { return d > 0; });

    if (found == distribution.end()) {
      return samples;
    }
  }

  std::vector<unsigned int> hit_per_object(distribution.size());
  random_multinomial(distribution.size(), size, distribution, hit_per_object);

  auto index = 0;
  for (auto i = 0; i < hit_per_object.size(); i++) {
    for (int j = 0; j < hit_per_object[i]; ++j) {
      samples[index] = all_objects[i];
      index++;
    }
  }
  if (is_shuffled) {
    shuffle(samples);
  }
  return samples;
}

/* Roulette sampling is without replacement, means 1 person can be selected multiple times */
template <class T>
std::vector<T *> utils::Random::roulette_sampling(int number_of_samples, std::vector<double> &distribution,
                                                  std::vector<T *> &all_objects, bool is_shuffled, double sum_distribution) {
  if (all_objects.empty() || distribution.empty()) {
    spdlog::error("Error in roulette sampling. Empty distribution or all_objects.");
    return std::vector<T *>(number_of_samples, nullptr);
  }
  std::vector<T *> samples(number_of_samples, nullptr);
  double sum { sum_distribution };
  if (sum_distribution == 0) {
    return samples;
  } else if (sum_distribution < 0) {
    sum = 0;
    for (auto d : distribution) {
      sum += d;
    }
  }

  std::vector<double> uniform_sampling(number_of_samples, 0.0);
  for (auto &index : uniform_sampling) {
    index = this->random_uniform() * sum;
  }

  std::sort(uniform_sampling.begin(), uniform_sampling.end());

  double sum_weight = 0;
  int uniform_sampling_index = 0;

  for (auto pi = 0; pi < distribution.size(); pi++) {
    if (distribution[pi] == 0) continue;
    sum_weight += distribution[pi];
    while (uniform_sampling_index < number_of_samples && uniform_sampling[uniform_sampling_index] < sum_weight) {
      samples[uniform_sampling_index] = all_objects[pi];
      uniform_sampling_index++;
    }
    if (uniform_sampling_index == number_of_samples) {
      break;
    }
  }

  if (uniform_sampling_index < number_of_samples) {
    spdlog::error("Error in roulette sampling. Sum weight: {}. Sum distribution: {}", sum_weight, sum_distribution);
  }

  if (is_shuffled) {
    shuffle(samples);
  }
  return samples;
}

template <class T>
std::vector<std::tuple<T *, double>> utils::Random::roulette_sampling_tuple(int number_of_samples,
                                                                            std::vector<double> &distribution,
                                                                            std::vector<T *> &all_objects, bool is_shuffled,
                                                                            double sum_distribution) {
  std::vector<std::tuple<T *, double>> samples(number_of_samples, std::make_tuple(nullptr, 0.0));
  double sum { sum_distribution };
  if (sum_distribution == 0) {
    return samples;
  } else if (sum_distribution < 0) {
    sum = 0;
    for (auto d : distribution) {
      sum += d;
    }
  }

  std::vector<double> uniform_sampling(number_of_samples, 0.0);
  for (auto &index : uniform_sampling) {
    index = this->random_uniform() * sum;
  }

  std::sort(uniform_sampling.begin(), uniform_sampling.end());

  double sum_weight = 0;
  int uniform_sampling_index = 0;

  for (auto pi = 0; pi < distribution.size(); pi++) {
    if (distribution[pi] == 0) continue;
    sum_weight += distribution[pi];
    while (uniform_sampling_index < number_of_samples && uniform_sampling[uniform_sampling_index] < sum_weight) {
      samples[uniform_sampling_index] = std::make_tuple(all_objects[pi], distribution[pi]);
      uniform_sampling_index++;
    }
    if (uniform_sampling_index == number_of_samples) {
      break;
    }
  }

  if (uniform_sampling_index < number_of_samples) {
    spdlog::error("Error in roulette sampling tuple. Sum weight: {}. Sum distribution: {}", sum_weight, sum_distribution);
  }

  if (is_shuffled) {
    shuffle(samples);
  }
  return samples;
}
#endif  // RANDOM_H

