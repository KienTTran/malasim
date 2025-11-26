#ifndef PERSON_TEST_BASE_H
#define PERSON_TEST_BASE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Configuration/Config.h"
#include "Configuration/PopulationDemographic.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/Person/Person.h"
#include "Population/Population.h"
#include "Simulation/Model.h"
#include "Utils/Random.h"
#include "date/date.h"

using namespace testing;

// Mock classes, if there is no mock method, it is fake class
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
    // configure_immune_system(person_.get(), 0.5);  // Default probability
    
    PopulationDemographic population_demographic;
    auto default_age_structure = std::vector<int>{5, 15, 30, 50, 70, 90};
    population_demographic.set_age_structure(default_age_structure);
    population_demographic.set_mortality_when_treatment_fail_by_age_class(std::vector<double>(6, 0.4));
    population_demographic.set_death_rate_by_age_class(std::vector<double>(6, 0.2));
    set_population_demographic(population_demographic);

    ImmuneSystemParameters immune_system_parameters;
    set_immune_system_parameters(immune_system_parameters);

    ParasiteParameters parasite_parameters;
    // use the default values
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
              (Person*, const Person::Property &, const void*, const void*));
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
  // MOCK_METHOD(int, random_uniform, (int, int), (override));
  // MOCK_METHOD(double, random_beta, (double, double), (override));
  // MOCK_METHOD(double, random_gamma, (double, double), (override));
};

class PersonTestBase : public ::testing::Test {
protected:

  void SetUp() override {
    // Get the Model instance, DO NOT initialize it here
    original_model_ = Model::get_instance();
    original_model_->initialize();

    original_model_->set_config(std::make_unique<MockConfig>()); 
    mock_config_ = static_cast<MockConfig*>(original_model_->get_config()); 

    original_model_->set_scheduler(std::make_unique<MockScheduler>());
    mock_scheduler_ = static_cast<MockScheduler*>(original_model_->get_scheduler());

    original_model_->set_random(std::make_unique<MockRandom>());
    mock_random_ = static_cast<MockRandom*>(original_model_->get_random());

    original_model_->set_population(std::make_unique<MockPopulation>());
    mock_population_ = static_cast<MockPopulation*>(original_model_->get_population());

    // Create person instance and initialize it (it should use the correctly configured mocks)
    person_ = std::make_unique<Person>();
    person_->set_population(mock_population_); 
    person_->initialize(); 
    person_->set_immune_system(std::make_unique<MockImmuneSystem>(person_.get()));
    mock_immune_system_ = static_cast<MockImmuneSystem*>(person_->get_immune_system());
  }

  void TearDown() override {
    person_.reset();
    original_model_->release();
  }

protected:
  std::unique_ptr<Person> person_;
  Model* original_model_;
  MockConfig* mock_config_;
  MockScheduler* mock_scheduler_;
  MockRandom* mock_random_;
  MockPopulation* mock_population_;
  MockImmuneSystem* mock_immune_system_;
};

#endif  // PERSON_TEST_BASE_H

