# Test Writing Guide

This guide explains how to write tests in the MalaSim project using the new infrastructure that eliminates external file dependencies.

## Overview

The test infrastructure provides three approaches depending on your needs:

1. **Mock-Only Tests** - Pure unit tests with no file I/O
2. **Configuration Tests** - Tests for YAML parsing with in-memory data
3. **Integration-Style Tests** - Tests requiring full configuration (generated programmatically)

## Quick Start

### 1. Mock-Only Tests (Recommended for Unit Tests)

Use this for testing individual components without file dependencies.

```cpp
#include <gtest/gtest.h>
#include "fixtures/MockFactories.h"

class MyComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up Model with mocks
        auto* model = Model::get_instance();
        model->release();
        auto mocks = test_fixtures::setup_model_with_mocks(model);
        
        // Store mock pointers if needed for assertions
        mock_config_ = mocks.config;
        mock_scheduler_ = mocks.scheduler;
        mock_random_ = mocks.random;
        mock_population_ = mocks.population;
    }

    void TearDown() override {
        Model::get_instance()->release();
    }

protected:
    MockConfig* mock_config_;
    MockScheduler* mock_scheduler_;
    MockRandom* mock_random_;
    MockPopulation* mock_population_;
};

TEST_F(MyComponentTest, BasicFunctionality) {
    // Your test here - uses mocks, no external files
}
```

**When to use:**
- Testing individual classes or functions
- No need for complex configuration
- Want fast, isolated tests
- Testing logic, not file I/O

### 2. Configuration Tests (YAML Parsing)

Use this for testing configuration parsing without actual files.

```cpp
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "fixtures/InMemoryYamlConfig.h"

TEST(ConfigurationTest, ParseSpatialSettings) {
    // Create YAML in memory
    std::string yaml_str = test_fixtures::yaml_configs::minimal_grid_based_spatial();
    YAML::Node node = YAML::Load(yaml_str);
    
    // Test parsing
    SpatialSettings settings;
    EXPECT_NO_THROW(YAML::convert<SpatialSettings>::decode(node, settings));
    
    // Verify results
    EXPECT_EQ(settings.get_mode(), "grid_based");
}
```

**When to use:**
- Testing YAML parsing logic
- Testing configuration validation
- No need to load actual raster files
- Testing serialization/deserialization

### 3. Integration-Style Tests (Full Configuration)

Use this when you need a complete Model initialization with configuration files.

```cpp
#include <gtest/gtest.h>
#include "fixtures/TestFileGenerators.h"
#include "Utils/Cli.h"
#include "Simulation/Model.h"

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate test environment from template
        test_fixtures::create_complete_test_environment();
        
        // Load the generated config
        utils::Cli::get_instance().set_input_path("test_input.yml");
        ASSERT_TRUE(Model::get_instance()->initialize());
    }

    void TearDown() override {
        // Clean up generated files
        test_fixtures::cleanup_test_files();
    }
};

TEST_F(IntegrationTest, FullModelInitialization) {
    // Test with fully initialized model
    EXPECT_NE(Model::get_config(), nullptr);
    EXPECT_NE(Model::get_scheduler(), nullptr);
}
```

**When to use:**
- Testing components that require full Model initialization
- Testing interactions between multiple subsystems
- Need drug database, genotype database, spatial data, etc.
- Testing end-to-end workflows

### 4. Custom Configuration Tests

Modify the configuration programmatically for specific test scenarios:

```cpp
#include "fixtures/TestFileGenerators.h"

class CustomConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate with custom modifications
        test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node& config) {
            // Customize configuration
            config["simulation_timeframe"]["ending_date"] = "2005/1/1";
            config["model_settings"]["initial_seed_number"] = 42;
            config["drug_db"][0]["drug_half_life"] = 2.0;
            
            // Change to location-based mode (no rasters needed)
            config["spatial_settings"]["mode"] = "location_based";
        });
        
        utils::Cli::get_instance().set_input_path("test_input.yml");
        Model::get_instance()->initialize();
    }

    void TearDown() override {
        test_fixtures::cleanup_test_files();
    }
};
```

## Available Mock Factories

### Creating Mock Objects

```cpp
// Create minimal config
auto config = test_fixtures::create_minimal_config();

// Create config with custom time
auto config = test_fixtures::create_config_with_time(365);

// Create config with custom age structure
auto config = test_fixtures::create_config_with_ages({5, 15, 30, 50});

// Create mock immune system
auto immune = test_fixtures::create_mock_immune_system(person_ptr);

// Set up complete model with mocks
auto mocks = test_fixtures::setup_model_with_mocks(model);
```

## In-Memory YAML Configurations

Available pre-built YAML strings in `InMemoryYamlConfig.h`:

```cpp
test_fixtures::yaml_configs::minimal_grid_based_spatial()
test_fixtures::yaml_configs::minimal_location_based_spatial()
test_fixtures::yaml_configs::minimal_population_demographic()
test_fixtures::yaml_configs::minimal_epidemiological_parameters()
test_fixtures::yaml_configs::minimal_drug_parameters()
test_fixtures::yaml_configs::minimal_therapy()
test_fixtures::yaml_configs::minimal_seasonality()
test_fixtures::yaml_configs::minimal_simulation_timeframe()
test_fixtures::yaml_configs::minimal_model_settings()
```

## File Generation Details

### How TestFileGenerators Works

1. **Template Loading:** Loads `test_input_template.yml` (full configuration with `test_*` file paths)
2. **Modification:** Optionally modifies with yaml-cpp
3. **YAML Writing:** Writes `test_input.yml` to test directory
4. **Raster Generation:** Automatically creates all `.asc` files referenced in YAML
5. **CSV Generation:** Creates seasonality CSV files

### Generated Files

The system automatically creates:
- `test_input.yml` - Complete configuration
- `test_init_pop.asc` - Population raster (10x10, 1000 people/cell)
- `test_district.asc` - Administrative boundaries (10x10, district ID 1)
- `test_treatment.asc` - Treatment coverage (10x10, 0.6)
- `test_beta.asc` - Transmission intensity (10x10, 0.5)
- `test_ecozone.asc` - Ecoclimatic zones (10x10, zone 1)
- `test_travel.asc` - Travel data (10x10, 0.1)
- `test_mosquito_ifr.asc` - Mosquito feeding rate (10x10, 0.1)
- `test_mosquito_size.asc` - Mosquito population (10x10, 100000)
- `test_seasonality_pattern.csv` - Monthly seasonality data
- `test_seasonality.csv` - Alternative seasonality data

All generated files have minimal but valid data for testing.

## Migration Guide

### Migrating Existing Tests

**Old pattern:**
```cpp
void SetUp() override {
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    Model::get_instance()->initialize();
}
```

**New pattern:**
```cpp
#include "fixtures/TestFileGenerators.h"

void SetUp() override {
    test_fixtures::create_complete_test_environment();
    utils::Cli::get_instance().set_input_path("test_input.yml");
    Model::get_instance()->initialize();
}

void TearDown() override {
    test_fixtures::cleanup_test_files();
}
```

That's it! The test now generates its own files instead of depending on `sample_inputs/`.

## Best Practices

### 1. Choose the Right Approach

- **Mock-Only** for pure unit tests (fastest, most isolated)
- **In-Memory YAML** for configuration parsing tests
- **File Generation** only when you truly need full Model initialization

### 2. Clean Up

Always call `cleanup_test_files()` in `TearDown()` if you generated files:

```cpp
void TearDown() override {
    // Your cleanup
    test_fixtures::cleanup_test_files();
}
```

### 3. Test Isolation

Each test should generate its own files to avoid interference:

```cpp
// GOOD: Each test generates fresh files
TEST_F(MyTest, Test1) { /* Uses fresh test_input.yml */ }
TEST_F(MyTest, Test2) { /* Gets new test_input.yml in SetUp */ }

// BAD: Sharing generated files between tests
// (Don't do this - let SetUp/TearDown handle it)
```

### 4. Minimal Configuration

Generate only what you need. If testing doesn't require spatial data, consider using location-based mode:

```cpp
test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node& config) {
    config["spatial_settings"]["mode"] = "location_based";
    // No raster files needed!
});
```

### 5. Debugging Generated Files

If a test fails, the generated files remain in the test directory. You can inspect:
- `build/bin/test_input.yml` - The configuration
- `build/bin/test_*.asc` - The raster files

## Examples

### Example 1: Pure Unit Test (No Files)

```cpp
#include "fixtures/MockFactories.h"

TEST(DrugCalculationTest, CalculateConcentration) {
    // Create drug with known parameters
    DrugType drug_type;
    drug_type.set_drug_half_life(1.0);
    
    // Test calculation (no Model needed)
    double concentration = calculate_concentration(1.0, 1.0, 1.0);
    EXPECT_NEAR(concentration, 0.5, 0.01);
}
```

### Example 2: Configuration Test

```cpp
#include "fixtures/InMemoryYamlConfig.h"

TEST(SpatialConfigTest, ParseGridBased) {
    std::string yaml = test_fixtures::yaml_configs::minimal_grid_based_spatial();
    YAML::Node node = YAML::Load(yaml);
    
    SpatialSettings settings;
    YAML::convert<SpatialSettings>::decode(node, settings);
    
    EXPECT_EQ(settings.get_mode(), "grid_based");
}
```

### Example 3: Integration Test

```cpp
#include "fixtures/TestFileGenerators.h"

class DrugDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_fixtures::create_complete_test_environment();
        utils::Cli::get_instance().set_input_path("test_input.yml");
        Model::get_instance()->initialize();
    }
    
    void TearDown() override {
        test_fixtures::cleanup_test_files();
    }
};

TEST_F(DrugDatabaseTest, LoadsDrugs) {
    EXPECT_GT(Model::get_drug_db()->size(), 0);
}
```

## Troubleshooting

### "Failed to load test_input_template.yml"

The template file needs to be copied to the test directory during build. Check:
1. File exists: `tests/fixtures/test_input_template.yml`
2. CMakeLists.txt copies it: `add_custom_command` to copy template
3. Rebuild: `make clean && make test`

### "File not found: test_*.asc"

The raster files should be auto-generated. Check:
1. `create_complete_test_environment()` is called in `SetUp()`
2. Template YAML references the correct filenames
3. Write permissions in test directory

### Tests Fail After Changing Template

If you modify `test_input_template.yml`:
1. Rebuild to copy new template: `make test`
2. Or manually copy: `cp tests/fixtures/test_input_template.yml build/bin/`

## File Structure

```
tests/
├── fixtures/
│   ├── MockFactories.h              # Mock objects and factories
│   ├── InMemoryYamlConfig.h         # In-memory YAML strings
│   ├── TestFileGenerators.h         # File generation utilities
│   └── test_input_template.yml      # Complete config template
├── Population/
│   ├── Person/
│   │   ├── PersonTestBase.h         # Uses MockFactories
│   │   └── *.cpp                    # Person tests
│   └── PopulationGenerateIndividualTest.cpp  # Uses TestFileGenerators
├── Configuration/
│   └── yaml_*_test.cpp              # Use InMemoryYamlConfig
└── Treatment/
    └── *.cpp                        # Most can use TestFileGenerators
```

## Summary

The new test infrastructure provides:
- ✅ **No external dependencies** - Tests generate their own files
- ✅ **Maintainable** - Single template, clear modifications
- ✅ **Flexible** - yaml-cpp for programmatic config changes
- ✅ **Fast** - Mocks for pure unit tests, generated files for integration
- ✅ **Isolated** - Each test gets a clean environment

Choose the approach that fits your test needs, and tests will be more reliable and maintainable!
