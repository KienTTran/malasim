# Population

This module implements the core population dynamics and individual-based modeling components of the malaria simulation, including host populations, parasite populations within hosts, and various biological processes.

## Overview

The Population module is responsible for:
- Managing host populations and demographics
- Handling parasite populations within hosts
- Implementing infection dynamics
- Managing population movements
- Tracking immune system states
- Handling drug interactions

## Key Components

### Population Management
- Population initialization and updates
- Birth and death events
- Location-based population tracking
- Age structure management
- Host state tracking

### Individual Management (`Person/`)
- Individual characteristics
- Health status tracking
- Movement patterns
- Treatment history
- Immune status

### Parasite Populations
- `ClonalParasitePopulation`: Single genotype parasites
- `SingleHostClonalParasitePopulations`: Multiple genotypes in one host
- Density calculations
- Drug interactions
- Mutation tracking

### Immune System (`ImmuneSystem/`)
- Immune response modeling
- Immunity acquisition
- Protection calculations
- Age-dependent immunity
- Exposure history

### Drug Interactions
- `DrugsInBlood`: Drug concentration tracking
- Treatment effects
- Drug resistance interactions
- Metabolism modeling
- Treatment outcomes

## Data Structures

### Population Organization
```cpp
class Population {
    Model* model_;
    PersonIndexPtrList* person_index_list_;
    PersonIndexAll* all_persons_;
    IntVector popsize_by_location_;
    
    // Force of Infection tracking
    std::vector<std::vector<double>> individual_foi_by_location;
    std::vector<double> current_force_of_infection_by_location;
};
```

### Parasite Management
```cpp
class SingleHostClonalParasitePopulations {
    std::vector<ClonalParasitePopulation*> parasites_;
    double log10_total_density_;
    double log10_total_infectious_density_;
    int last_update_log10_total_density_;
};
```

## Key Features

### Population Dynamics
- Birth rates and demographics
- Mortality calculations
- Age structure maintenance
- Location-based dynamics
- Population movement

### Infection Dynamics
- Force of infection calculations
- Transmission modeling
- Clinical progression
- Treatment outcomes
- Immunity effects

### Movement Patterns
- Location-based circulation
- Movement rates
- Residence tracking
- Travel patterns
- Population mixing

## Usage

### Population Management
```cpp
// Initialize population
population->initialize();

// Add new individual
population->add_person(person);

// Handle demographic events
population->perform_birth_event();
population->perform_death_event();

// Update population state
population->update_all_individuals();
population->update_current_foi();
```

### Infection Management
```cpp
// Introduce parasites
population->introduce_parasite(
    location,
    parasite_type,
    num_infections
);

// Handle infection events
population->perform_infection_event();

// Check infection status
population->has_0_case();
```

## Implementation Details

### Population Indexing
- Multiple indexing systems
- Location-based indexing
- Age-class indexing
- State-based indexing
- Efficient lookup mechanisms

### Event Handling
- Birth events
- Death events
- Movement events
- Infection events
- Treatment events

### Performance Considerations
- Efficient data structures
- Optimized lookups
- Memory management
- Event scheduling
- State updates

## Dependencies

- Core simulation components:
  - `Model`
  - `Config`
  - `Random`
- Person-related classes:
  - `Person`
  - `PersonIndex`
- Disease components:
  - `Genotype`
  - `DrugDatabase`
  - `ImmuneSystem`

## Notes

- Population size is dynamic
- Multiple indexing systems enable efficient lookups
- Memory management is critical for large populations
- Event handling follows biological patterns
- Location-based dynamics affect transmission
- Immune system modeling is age-dependent
- Drug interactions affect treatment outcomes
- Movement patterns influence disease spread
