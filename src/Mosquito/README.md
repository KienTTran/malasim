# Mosquito

This module implements the mosquito population dynamics and parasite transmission between mosquitoes and humans in the malaria simulation model.

## Overview

The Mosquito module is responsible for:
- Managing mosquito populations by location
- Simulating parasite transmission dynamics
- Handling interrupted feeding events
- Implementing genetic recombination of parasites
- Tracking drug resistance evolution

## Key Features

### Population Management
- Location-specific mosquito populations
- Dynamic population sizing
- Cohort-based tracking
- Temporal population changes

### Transmission Dynamics
- Human-to-mosquito transmission
- Interrupted feeding simulation
- Infection probability calculations
- Force of infection modeling

### Genetic Components
- Parasite genotype tracking
- Recombination implementation
- Drug resistance monitoring
- Mutation tracking

### Recombination Mechanisms

#### Within-Host Recombination
Four possible scenarios are implemented:
1. Within-Host=1, Interrupted Feeding=1
   - Recombination between parasites from two different persons
   - Multiple genotype sampling from each person
   - Complex recombination patterns

2. Within-Host=1, Interrupted Feeding=0
   - Recombination within a single person
   - Multiple genotype sampling
   - Internal recombination

3. Within-Host=0, Interrupted Feeding=1
   - Simple recombination between two persons
   - Single genotype from each person
   - Direct recombination

4. Within-Host=0, Interrupted Feeding=0
   - Single person, single genotype
   - No effective recombination

### Drug Resistance Tracking
- Monitors resistance evolution
- Tracks specific drug combinations:
  - DHA-PPQ (Dihydroartemisinin-Piperaquine)
  - ASAQ (Artesunate-Amodiaquine)
  - AL (Artemether-Lumefantrine)
  - Combination therapies

## Usage

```cpp
// Initialize mosquito population
auto mosquito = Mosquito(model);
mosquito.initialize(config);

// Infect new mosquito cohort
mosquito.infect_new_cohort_in_PRMC(
    config,
    random,
    population,
    tracking_index
);

// Get random genotype from location
int genotype_index = mosquito.random_genotype(location, tracking_index);
```

## Implementation Details

### Data Structures
- 3D vector for genotype tracking:
  - Dimension 1: Tracking days
  - Dimension 2: Locations
  - Dimension 3: Individual mosquitoes

### Key Algorithms
- Roulette wheel sampling for transmission
- Interrupted feeding probability calculations
- Genetic recombination implementation
- Drug resistance evolution tracking

### Performance Considerations
- Efficient genotype storage
- Optimized sampling algorithms
- Memory management for large populations
- Vector reuse for performance

## Dependencies

- Core simulation components:
  - `Model`
  - `Config`
  - `Population`
  - `Random`
- Parasite-related classes:
  - `Genotype`
  - `ClonalParasitePopulation`
- Utility classes:
  - Vector types from `TypeDef.hxx`

## Notes

- Mosquito populations are managed by location
- Recombination events are tracked and logged
- Drug resistance patterns are monitored
- Interrupted feeding affects transmission dynamics
- Population sizes are configurable per location
- Genetic diversity is maintained through recombination
- Memory usage scales with population size and tracking days 