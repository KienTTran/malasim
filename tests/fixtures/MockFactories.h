#ifndef MOCK_FACTORIES_H
#define MOCK_FACTORIES_H

#include <gmock/gmock.h>
#include <memory>

#include "Configuration/Config.h"
#include "Configuration/EpidemiologicalParameters.h"
#include "Configuration/ImmuneSystemParameters.h"
#include "Configuration/ModelSettings.h"
#include "Configuration/ParasiteParameters.h"
#include "Configuration/PopulationDemographic.h"
#include "Configuration/SimulationTimeframe.h"
#include "Core/Scheduler/Scheduler.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Utils/Random.h"
#include "date/date.h"

using namespace testing;

// ============================================================================
// MOCK CLASSES
// ============================================================================

class MockConfig : public Config {
public:
  MockConfig() : Config() {
    // Set up simulation timeframe
    SimulationTimeframe simulation_timeframe_;
    simulation_timeframe_.set_total_time(1000);
    set_simulation_timeframe(simulation_timeframe_);

    ModelSettings model_settings;
    model_settings.set_cell_level_reporting(true);
    set_model_settings(model_settings);

    EpidemiologicalParameters epidemiological_parameters;
    epidemiological_parameters.set_days_to_clinical_under_five(5);
    epidemiological_parameters.set_days_to_clinical_over_five(7);
    epidemiological_parameters.set_days_mature_gametocyte_under_five(10);
    epidemiological_parameters.set_days_mature_gametocyte_over_five(14);
    set_epidemiological_parameters(epidemiological_parameters);

    PopulationDemographic population_demographic;
    auto default_age_structure = std::vector<int>{5, 15, 30, 50, 70, 90};
    population_demographic.set_age_structure(default_age_structure);
    population_demographic.set_mortality_when_treatment_fail_by_age_class(
        std::vector<double>(6, 0.4));
    population_demographic.set_death_rate_by_age_class(std::vector<double>(6, 0.2));
    set_population_demographic(population_demographic);

    ImmuneSystemParameters immune_system_parameters;
    set_immune_system_parameters(immune_system_parameters);

    ParasiteParameters parasite_parameters;
    set_parasite_parameters(parasite_parameters);
  }
};

class MockScheduler : public Scheduler {
public:
  explicit MockScheduler() : Scheduler() {}
  MOCK_METHOD(void, schedule_population_event, (WorldEvent*));
};

class MockPopulation : public Population {
public:
  explicit MockPopulation() : Population() {}
  MOCK_METHOD(void, notify_change,
              (Person*, const Person::Property&, const void*, const void*));
};

class MockImmuneSystem : public ImmuneSystem {
public:
  explicit MockImmuneSystem(Person* person) : ImmuneSystem(person) {}

  MOCK_METHOD(double, get_current_value, (), (const));
  MOCK_METHOD(double, get_clinical_progression_probability, (), (const));
  MOCK_METHOD(void, update, ());
  MOCK_METHOD(void, draw_random_immune, ());
};

class MockRandom : public utils::Random {
public:
  explicit MockRandom() : Random(nullptr) {}
  MOCK_METHOD(double, random_normal_double, (double mean, double standard_deviation), (override));
  MOCK_METHOD(int, random_normal_int, (int mean, double standard_deviation), (override));
  MOCK_METHOD(double, random_flat, (double, double), (override));
  MOCK_METHOD(uint64_t, random_uniform, (uint64_t), (override));
};

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

namespace test_fixtures {

/**
 * @brief Create a minimal MockConfig for basic unit tests
 * @return unique_ptr to MockConfig with default settings
 */
inline std::unique_ptr<MockConfig> create_minimal_config() {
  return std::make_unique<MockConfig>();
}

/**
 * @brief Create a MockConfig with custom simulation time
 * @param total_days Total simulation days
 * @return unique_ptr to MockConfig
 */
inline std::unique_ptr<MockConfig> create_config_with_time(int total_days) {
  auto config = std::make_unique<MockConfig>();
  SimulationTimeframe timeframe;
  timeframe.set_total_time(total_days);
  config->set_simulation_timeframe(timeframe);
  return config;
}

/**
 * @brief Create a MockConfig with custom age structure
 * @param age_structure Vector of age class boundaries
 * @return unique_ptr to MockConfig
 */
inline std::unique_ptr<MockConfig> create_config_with_ages(
    const std::vector<int>& age_structure) {
  auto config = std::make_unique<MockConfig>();
  PopulationDemographic pop_demo;
  pop_demo.set_age_structure(age_structure);
  pop_demo.set_mortality_when_treatment_fail_by_age_class(
      std::vector<double>(age_structure.size(), 0.4));
  pop_demo.set_death_rate_by_age_class(std::vector<double>(age_structure.size(), 0.2));
  config->set_population_demographic(pop_demo);
  return config;
}

/**
 * @brief Initialize Model with standard mocks for unit testing
 * @param model Pointer to Model instance to configure
 * @return Struct containing pointers to all mocks
 */
struct ModelMocks {
  MockConfig* config;
  MockScheduler* scheduler;
  MockPopulation* population;
  MockRandom* random;
};

inline ModelMocks setup_model_with_mocks(Model* model) {
  model->initialize();

  // Set up config
  model->set_config(std::make_unique<MockConfig>());
  auto* mock_config = static_cast<MockConfig*>(model->get_config());

  // Set up scheduler
  model->set_scheduler(std::make_unique<MockScheduler>());
  auto* mock_scheduler = static_cast<MockScheduler*>(model->get_scheduler());
  mock_scheduler->initialize(date::year_month_day{date::year{2000}, date::month{1}, date::day{1}},
                             date::year_month_day{date::year{2010}, date::month{12}, date::day{31}});

  // Set MDC to nullptr to avoid unwanted side effects
  model->set_mdc(nullptr);

  // Set up random
  model->set_random(std::make_unique<MockRandom>());
  auto* mock_random = static_cast<MockRandom*>(model->get_random());

  // Set up population
  model->set_population(std::make_unique<MockPopulation>());
  auto* mock_population = static_cast<MockPopulation*>(model->get_population());

  return ModelMocks{mock_config, mock_scheduler, mock_population, mock_random};
}

/**
 * @brief Create a MockImmuneSystem for a Person
 * @param person Pointer to Person instance
 * @return unique_ptr to MockImmuneSystem
 */
inline std::unique_ptr<MockImmuneSystem> create_mock_immune_system(Person* person) {
  return std::make_unique<MockImmuneSystem>(person);
}

}  // namespace test_fixtures

#endif  // MOCK_FACTORIES_H
