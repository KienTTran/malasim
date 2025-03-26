# Model Data Collector (MDC)

This module provides comprehensive data collection and statistical analysis functionality for the simulation model. It tracks and analyzes various metrics related to disease progression, treatment outcomes, and population dynamics.

## Overview

The Model Data Collector (MDC) is responsible for:
- Collecting real-time simulation statistics
- Tracking population-level metrics
- Monitoring disease progression
- Analyzing treatment effectiveness
- Recording mutation events
- Calculating epidemiological indicators

## Key Features

### Population Statistics
- Population size by location
- Age class distribution
- Host state tracking
- Birth and death rates
- Movement patterns

### Disease Metrics
- Blood slide prevalence
- Clinical episodes
- Infection rates
- Parasite populations
- Gametocyte carriers
- Multiple infections (MOI)

### Treatment Monitoring
- Treatment coverage
- Treatment failures (TF)
- Drug resistance
- Mass Drug Administration (MDA) outcomes
- Therapy effectiveness
- Antimalarial usage (AMU)
- Antimalarial failure (AFU)

### Transmission Dynamics
- Entomological Inoculation Rate (EIR)
- Bite distribution
- Mosquito-human interaction
- Seasonal patterns
- Location-specific transmission

### Resistance Tracking
- Mutation events
- Resistance frequencies
- Multiple resistance patterns
- Recombination events
- Drug-specific resistance

## Data Collection Categories

### Temporal Resolution
- Daily statistics
- Monthly aggregation
- Yearly summaries
- Rolling averages (e.g., last 10 time periods)

### Spatial Resolution
- Location-based statistics
- Age-stratified data
- Host-state specific metrics
- Treatment-specific outcomes

### Clinical Outcomes
- Clinical episodes
- Treatment success/failure
- Mortality rates
- Drug resistance impact
- Untreated cases

## Usage

```cpp
// Initialize the data collector
auto collector = ModelDataCollector();
collector.initialize();

// Record events during simulation
collector.collect_1_clinical_episode(location, age, age_class);
collector.record_1_treatment(location, age, age_class, therapy_id);
collector.record_1_mutation(location, from_genotype, to_genotype);

// Perform periodic updates
collector.monthly_update();
collector.yearly_update();

// Calculate statistics
collector.calculate_eir();
collector.calculate_percentage_bites_on_top_20();

// Get specific metrics
double prevalence = collector.get_blood_slide_prevalence(location, age_from, age_to);
```

## Implementation Details

### Data Storage
- Uses various vector types for different metrics:
  - `IntVector`: Integer-based counts
  - `DoubleVector`: Floating-point measurements
  - `LongVector`: Large number counts
  - Multi-dimensional vectors for complex metrics

### Update Mechanisms
- Begin/end time step updates
- Monthly statistical calculations
- Yearly aggregations
- Event-based recording

### Performance Considerations
- Efficient data structures
- Optimized calculation methods
- Memory-conscious design
- Thread-safe operations

## Dependencies

- Core simulation components:
  - `Model`
  - `Person`
  - `Therapy`
  - `ClonalParasitePopulation`
  - `Genotype`
- Utility classes:
  - Vector types from `TypeDef.hxx`

## Notes

- All statistics are updated in real-time during simulation
- Data can be accessed for analysis at any time point
- Supports both aggregate and detailed statistics
- Designed for extensibility with new metrics
- Critical for validation and analysis of simulation outcomes
- Memory usage scales with simulation size and complexity 