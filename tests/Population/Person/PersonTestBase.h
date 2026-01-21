#ifndef PERSON_TEST_BASE_H
#define PERSON_TEST_BASE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "fixtures/MockFactories.h"

using namespace testing;

class PersonTestBase : public ::testing::Test {
protected:

  void SetUp() override {
    // Get the Model instance and set up with standard mocks
    original_model_ = Model::get_instance();
    auto mocks = test_fixtures::setup_model_with_mocks(original_model_);
    
    // Store mock pointers for test assertions
    mock_config_ = mocks.config;
    mock_scheduler_ = mocks.scheduler;
    mock_random_ = mocks.random;
    mock_population_ = mocks.population;

    // Create person instance and initialize it
    person_ = std::make_unique<Person>();
    person_->set_population(mock_population_);
    person_->initialize();
    person_->set_immune_system(test_fixtures::create_mock_immune_system(person_.get()));
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

