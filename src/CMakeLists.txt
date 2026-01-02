set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

find_package(fmt CONFIG REQUIRED)
find_package(GSL REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(spdlog REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(Torch CONFIG REQUIRED)

option(ENABLE_TRAVEL_TRACKING "Enable tracking of individual travel data and generating travel reports" OFF)

if(ENABLE_TRAVEL_TRACKING)
  message(STATUS "Travel tracking feature is enabled")
  # Add definitions or configurations specific to the travel tracking feature
  # add_definitions(-DENABLE_TRAVEL_TRACKING)
endif()

# include_directories(
#   ${PROJECT_SOURCE_DIR}/src
# )

# craete source files
# Add source files for the core library
file(GLOB_RECURSE MALASIM_CORE_SOURCES
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.hxx"
  "${PROJECT_SOURCE_DIR}/src/*.h"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/malasim/version_generated.h")
  message(STATUS "Using generated version_generated.h")
else()
  message(STATUS "Using default version.h")
endif()


# remove malasim/main.cpp from list
list(FILTER MALASIM_CORE_SOURCES EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/malasim/main\\.cpp$")

# Add source files for the core library
add_library(MalaSimCore STATIC
    # Add your source files here
    ${MALASIM_CORE_SOURCES}
)

target_include_directories(MalaSimCore PUBLIC "${PROJECT_SOURCE_DIR}/src")

target_compile_definitions(MalaSimCore PUBLIC
#        GLOG_USE_GLOG_EXPORT
        GLOG_NO_ABBREVIATED_SEVERITIES
)

if(ENABLE_TRAVEL_TRACKING)
  target_compile_definitions(MalaSimCore PUBLIC ENABLE_TRAVEL_TRACKING)
endif()

# Add coverage flags to MalaSimCore if enabled
if(ENABLE_COVERAGE)
  message(STATUS "LLVM Code coverage enabled for MalaSimCore")
  # Generate profile data and coverage mapping
  target_compile_options(MalaSimCore PRIVATE -fprofile-instr-generate -fcoverage-mapping)
  target_link_options(MalaSimCore PRIVATE -fprofile-instr-generate) # Might need link options too
endif()

# Link the core library and fmt to the main executable
target_link_libraries(MalaSimCore
        PUBLIC
        GSL::gsl
        GSL::gslcblas
        fmt::fmt-header-only
        yaml-cpp::yaml-cpp
        spdlog::spdlog
        date::date
        date::date-tz
        CLI11::CLI11
        unofficial::sqlite3::sqlite3
        torch
)

set_property(TARGET MalaSimCore PROPERTY CXX_STANDARD 20)

# Add the main executable
add_executable(MalaSim
    malasim/main.cpp
)

add_dependencies(MalaSim MalaSimCore)


if(ENABLE_COVERAGE)
  message(STATUS "LLVM Code coverage enabled for malasim_test")
  # Add flags for compiling the test executable itself
  target_compile_options(MalaSim PRIVATE -fprofile-instr-generate -fcoverage-mapping)
  # Add flags needed during linking
  target_link_options(MalaSim PRIVATE -fprofile-instr-generate)
endif()
# Link the core library and fmt to the main executable
target_link_libraries(MalaSim PRIVATE
  MalaSimCore
)

set_property(TARGET MalaSim PROPERTY CXX_STANDARD 20)

# Get full GitHub repository path (e.g., username/repo-name)
execute_process(
        COMMAND git config --get remote.origin.url
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REMOTE_URL
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Parse username and repo name
string(REGEX REPLACE ".*github.com[:/](.+)\\.git" "\\1" GIT_REPO_INFO "${GIT_REMOTE_URL}")

# Get current Git branch
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH_NAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get current Git commit hash (short)
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Define macros
add_definitions(-DGIT_REPO_INFO="${GIT_REPO_INFO}")
add_definitions(-DGIT_BRANCH_NAME="${GIT_BRANCH_NAME}")
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
