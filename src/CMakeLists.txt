find_package(fmt CONFIG REQUIRED)
find_package(GSL REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(spdlog REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

option(ENABLE_TRAVEL_TRACKING "Enable tracking of individual travel data and generating travel reports" OFF)

file(GLOB_RECURSE MALASIM_CORE_SOURCES
  CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER MALASIM_CORE_SOURCES EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/apps/")

# Add source files for the core library
add_library(MalaSimCore STATIC
    # Add your source files here
    ${MALASIM_CORE_SOURCES}

)

if(ENABLE_TRAVEL_TRACKING)
  message(STATUS "Travel tracking feature is enabled")
  # Add definitions or configurations specific to the travel tracking feature
  # add_definitions(-DENABLE_TRAVEL_TRACKING)
  target_compile_definitions(MalaSimCore PUBLIC ENABLE_TRAVEL_TRACKING)
endif()

target_include_directories(MalaSimCore PUBLIC "${PROJECT_SOURCE_DIR}/src")

if(ENABLE_PCH)
  target_precompile_headers(MalaSimCore PRIVATE <Simulation/Model.h>)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(MalaSimCore PRIVATE SHELL:-Xclang -fno-pch-timestamp)
  endif()
endif()

# Add coverage flags to MalaSimCore if enabled
if(ENABLE_COVERAGE)
  message(STATUS "LLVM Code coverage enabled for MalaSimCore")
  # Generate profile data and coverage mapping
  target_compile_options(MalaSimCore PRIVATE -fprofile-instr-generate -fcoverage-mapping)
  target_link_options(MalaSimCore PRIVATE -fprofile-instr-generate) # Might need link options too
endif()

# Link the core library and fmt to the main executable
target_link_libraries(MalaSimCore PUBLIC
  fmt::fmt-header-only
  GSL::gsl GSL::gslcblas
  yaml-cpp::yaml-cpp
  spdlog::spdlog
  date::date date::date-tz
  CLI11::CLI11
  unofficial::sqlite3::sqlite3
)

set_property(TARGET MalaSimCore PROPERTY CXX_STANDARD 20)

if(NOT ENABLE_COVERAGE)
  add_subdirectory(apps/malasim)
  add_subdirectory(apps/efficacy_estimator)
endif()
