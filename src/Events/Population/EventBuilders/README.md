# Event Builders

This module provides builder classes for constructing complex population-level events in the malaria simulation.

## Overview

The Event Builders module provides:
- Factory methods for event creation
- Event configuration builders
- Parameter validation
- Event composition
- Complex event construction

## Components

### Mutant Event Builder
- `IntroduceMutantEventBuilder`: Constructs mutation events
  - Parasite mutation configuration
  - Location-specific settings
  - Timing parameters
  - Population targeting
  - Resistance characteristics

## Key Features

### Event Construction
- Parameter validation
- Default value handling
- Configuration checking
- Error detection
- Event composition

### Mutation Events
- Parasite mutation events
- Resistance development
- Location-based mutations
- Timed introductions
- Population effects

## Implementation

### Builder Pattern
```cpp
class IntroduceMutantEventBuilder {
    // Configuration
    void set_mutation_type();
    void set_location();
    void set_timing();
    
    // Validation
    bool validate_parameters();
    
    // Construction
    Event* build();
};
```

## Usage

```cpp
// Create builder
auto builder = new IntroduceMutantEventBuilder();

// Configure event
builder->set_mutation_type(mutation_type);
builder->set_location(location);
builder->set_timing(start_time);

// Build event
auto event = builder->build();
```

## Dependencies

- Core components:
  - `Event`
  - `Model`
  - `Population`
- Event types:
  - Mutation events
  - Population events
  - Time-based events

## Notes

- Validate all parameters
- Handle edge cases
- Check configurations
- Maintain consistency
- Document requirements
- Test thoroughly
- Consider performance
- Handle errors appropriately 