# Helper Utilities

This module provides various helper functions and utility classes used throughout the malaria simulation system.

## Overview

The Helper Utilities provide:
- String manipulation
- Time calculations
- OS operations
- Number handling
- Database access
- System utilities

## Components

### String Helpers
- `StringHelpers`: String manipulation utilities
  - Text processing
  - Format conversion
  - Pattern matching
  - String operations
  - Text validation

### Time Utilities
- `TimeHelpers`: Time-related functions
  - Date calculations
  - Time conversions
  - Duration handling
  - Calendar operations
  - Scheduling support

### System Utilities
- `OSHelpers`: Operating system operations
- `cpu.hxx`: CPU-related utilities
- `UniqueId`: ID generation

### Database Access
- `SQLiteDatabase`: SQLite database interface
  - Connection management
  - Query execution
  - Transaction handling
  - Error recovery
  - Result processing

### Number Processing
- `NumberHelpers`: Numerical operations
- `RunningMedian`: Statistical calculations

## Implementation

### String Processing
```cpp
class StringHelpers {
    string trim(const string& str);
    vector<string> split(const string& str, char delim);
    string join(const vector<string>& parts, char delim);
};
```

### Time Management
```cpp
class TimeHelpers {
    time_t convert_to_timestamp(const string& date);
    string format_time(time_t timestamp);
    int calculate_duration(time_t start, time_t end);
};
```

## Usage

### String Operations
```cpp
// Process strings
auto trimmed = StringHelpers::trim(input);
auto parts = StringHelpers::split(input, ',');
auto combined = StringHelpers::join(parts, '-');
```

### Time Handling
```cpp
// Handle time
auto timestamp = TimeHelpers::convert_to_timestamp(date_str);
auto formatted = TimeHelpers::format_time(timestamp);
auto duration = TimeHelpers::calculate_duration(start, end);
```

## Key Features

### String Processing
- Trimming operations
- Split and join
- Case conversion
- Pattern matching
- Format validation

### Time Management
- Date parsing
- Time formatting
- Duration calculation
- Calendar operations
- Time zone handling

### System Operations
- OS interaction
- CPU information
- File operations
- Process management
- System resources

### Database Operations
- Connection handling
- Query execution
- Result processing
- Error handling
- Transaction management

## Dependencies

### Core Requirements
- Standard libraries
- SQLite
- System APIs
- File system access

### Optional Components
- Time zone database
- Locale information
- CPU extensions
- Database drivers

## Notes

### Usage Guidelines
- Validate input
- Handle errors
- Check parameters
- Document usage
- Test edge cases
- Profile performance
- Clean up resources

### Best Practices
- Handle all errors
- Validate parameters
- Document assumptions
- Test thoroughly
- Profile critical paths
- Clean up resources
- Log important events
- Handle edge cases 