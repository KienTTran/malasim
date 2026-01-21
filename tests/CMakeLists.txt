# Find Google Test
find_package(GTest REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/tests)

file(GLOB_RECURSE MALASIM_TEST_SOURCES
  "*.cpp"
  "helpers/*.cpp"
  "Core/Random/*.cpp"
  "Core/Scheduler/*.cpp"
  "Core/Scheduler/EventManager/*.cpp"
  "Configuration/*.cpp"
  "Environment/*.cpp"
  "GIS/SpatialDataTest/*.cpp"
  "GIS/AdminLevelManagerTest/*.cpp"
  "GIS/SpatialDataTest/*.cpp"
  "Population/*.cpp"
  "Population/Person/*.cpp"
  "Spatial/Movement/*.cpp"
  "Treatment/*.cpp"
  "Treatment/Therapies/*.cpp"
  "Treatment/Strategies/*.cpp"
  "Parasites/*.cpp"
  "Utils/*.cpp"
)

# Add test executable
add_executable(malasim_test
  ${MALASIM_TEST_SOURCES}
)

add_dependencies( malasim_test MalaSimCore)

# Add coverage flags if enabled
if(ENABLE_COVERAGE)
  message(STATUS "LLVM Code coverage enabled for malasim_test")
  # Add flags for compiling the test executable itself
  target_compile_options(malasim_test PRIVATE -fprofile-instr-generate -fcoverage-mapping)
  # Add flags needed during linking
  target_link_options(malasim_test PRIVATE -fprofile-instr-generate)
endif()

# Link the core library and GTest to the test executable
target_link_libraries(malasim_test PRIVATE
  GTest::gtest_main GTest::gmock
  MalaSimCore
)

set_property(TARGET malasim_test PROPERTY CXX_STANDARD 20)

# Copy sample_inputs folder to binary dir for tests needing input files
add_custom_command(TARGET malasim_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/sample_inputs
        $<TARGET_FILE_DIR:malasim_test>/sample_inputs)

# Copy test_input_template.yml to binary dir for programmatic test generation
add_custom_command(TARGET malasim_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/tests/fixtures/test_input_template.yml
        $<TARGET_FILE_DIR:malasim_test>/test_input_template.yml)

add_test(NAME malasim_test COMMAND $<TARGET_FILE:malasim_test>)
set_tests_properties(malasim_test PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:malasim_test>)
