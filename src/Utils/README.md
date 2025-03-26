# Utils

This module provides essential utility functions, tools, and helper classes used throughout the malaria simulation system.

## Overview

The Utils module provides:
- Random number generation
- Type definitions
- Object pooling
- Logging facilities
- Helper functions
- Index management
- CLI tools

## Components

### Core Utilities
- `Random`: Advanced random number generation
- `TypeDef`: Common type definitions
- `ObjectPool`: Memory management
- `Logger`: Logging system
- `Constants`: System constants

### Distribution Tools
- `MultinomialDistributionGenerator`: Statistical distributions
- `MatrixWriter`: Data output tools
- `YamlFile`: Configuration handling

### CLI Tools
- `Cli`: Command line interface tools
- Parameter parsing
- Input validation
- Help generation

## Submodules

### Index System (`Index/`)
- Person indexing
- Location tracking
- State management
- Age classification
- Movement tracking

### Helpers (`Helpers/`)
- String manipulation
- Time utilities
- OS operations
- Number handling
- Database helpers

## Implementation

### Random Number Generation
```cpp
class Random {
    // Core functionality
    double random_uniform();
    double random_normal();
    int random_poisson();
    
    // Distribution sampling
    void sample_distribution();
    void generate_sequence();
};
```

### Object Pool Management
```cpp
template<typename T>
class ObjectPool {
    T* acquire();
    void release(T* obj);
    void cleanup();
};
```

## Usage

### Random Numbers
```cpp
// Generate random values
auto random = Random::getInstance();
double value = random->random_uniform();
int count = random->random_poisson(lambda);

// Sample from distribution
auto sample = random->sample_distribution(dist);
```

### Object Pooling
```cpp
// Create pool
auto pool = new ObjectPool<MyClass>();

// Acquire and release objects
auto obj = pool->acquire();
pool->release(obj);
```

## Key Features

### Random Generation
- Uniform distribution
- Normal distribution
- Poisson process
- Custom distributions
- Sequence generation

### Memory Management
- Object pooling
- Resource tracking
- Memory optimization
- Cleanup handling
- Reference counting

### Helper Functions
- String operations
- Time calculations
- OS interactions
- Number formatting
- Database operations

## Dependencies

### Core Requirements
- Standard libraries
- SQLite
- YAML parser
- System APIs

### Optional Components
- Statistical libraries
- Matrix operations
- File system tools
- Database drivers

## Notes

### Usage Guidelines
- Initialize early
- Handle errors
- Check parameters
- Validate input
- Document usage
- Test thoroughly
- Monitor performance
- Clean up resources

### Best Practices
- Use object pools
- Handle edge cases
- Validate random seeds
- Log important events
- Check return values
- Document changes
- Test distributions
- Profile performance 