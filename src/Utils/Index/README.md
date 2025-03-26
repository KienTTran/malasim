# Person Indexing

This module implements the person indexing system for efficient lookup and management of individuals in the malaria simulation.

## Overview

The Person Indexing module provides:
- Multi-dimensional person indexing
- Location-based tracking
- State-based classification
- Age group management
- Movement level tracking

## Components

### Core Indexing
- `PersonIndex`: Base indexing functionality
- `PersonIndexAll`: Complete population index
- `Indexer`: Generic indexing template

### Specialized Indexes
- `PersonIndexByLocationStateAgeClass`: Location/state/age indexing
- `PersonIndexByLocationMovingLevel`: Movement tracking index

### Index Handlers
- `PersonIndexAllHandler`: Global index management
- `PersonIndexByLocationStateAgeClassHandler`: Complex index handling
- `PersonIndexByLocationMovingLevelHandler`: Movement index management

## Implementation

### Base Index Structure
```cpp
class PersonIndex {
    virtual void add(Person* person);
    virtual void remove(Person* person);
    virtual void notify_change();
};
```

### Specialized Index
```cpp
class PersonIndexByLocationStateAgeClass : public PersonIndex {
    void add_person(Person* p);
    void remove_person(Person* p);
    size_t size(int location, int state, int age_class);
};
```

## Usage

### Basic Indexing
```cpp
// Add to index
index->add(person);

// Remove from index
index->remove(person);

// Handle changes
index->notify_change(person, property, old_value, new_value);
```

### Complex Queries
```cpp
// Query by multiple dimensions
auto count = location_state_age_index->size(
    location_id,
    state_id,
    age_class
);

// Get movement information
auto moving_count = moving_level_index->size(
    location_id,
    moving_level
);
```

## Key Features

### Index Management
- Fast lookup operations
- Multi-dimensional indexing
- Change notification
- Size tracking
- Property updates

### Location Tracking
- Geographic distribution
- Movement patterns
- Location changes
- Population density
- Migration flows

### State Classification
- Health state tracking
- Age group management
- Movement level monitoring
- Population segmentation
- Dynamic updates

## Dependencies

- Core components:
  - `Person`
  - `Population`
  - `Model`
- Utility classes:
  - `TypeDef`
  - `Constants`

## Notes

### Performance
- Optimize lookup operations
- Maintain index integrity
- Handle concurrent access
- Monitor memory usage
- Profile critical paths

### Implementation
- Thread safety important
- Validate all changes
- Handle edge cases
- Document assumptions
- Test thoroughly
- Update atomically 