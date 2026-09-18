add_executable(DxGGenerator
  main.cpp
  DxGCli.cpp
  IndividualsFileReporter.cpp
  PkPdReporter.cpp
)

target_link_libraries(DxGGenerator PRIVATE MalaSimCore)
target_include_directories(DxGGenerator PRIVATE "${PROJECT_SOURCE_DIR}/src")
set_property(TARGET DxGGenerator PROPERTY CXX_STANDARD 20)
