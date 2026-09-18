add_executable(MalaSim
  main.cpp
  MaSimCli.cpp
)

target_link_libraries(MalaSim PRIVATE MalaSimCore)
set_property(TARGET MalaSim PROPERTY CXX_STANDARD 20)

execute_process(
  COMMAND git config --get remote.origin.url
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_REMOTE_URL
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX REPLACE ".*github.com[:/](.+)\\.git" "\\1" GIT_REPO_INFO "${GIT_REMOTE_URL}")

execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_BRANCH_NAME
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_compile_definitions(MalaSim PRIVATE
  GIT_REPO_INFO="${GIT_REPO_INFO}"
  GIT_BRANCH_NAME="${GIT_BRANCH_NAME}"
  GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
)
