cmake_minimum_required(VERSION 3.20)
project(daggorath
  VERSION 0.1.0
  DESCRIPTION "Faithful preservation implementation of Dungeons of Daggorath (1983)"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(DAGGORATH_BUILD_TESTS "Build the conformance tests" ON)

# Where the Phase 0b fixtures live. The tests cross-check against these files,
# so they are located at configure time rather than copied into the build.
set(DAGGORATH_FIXTURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/archaeology/phase-0b/fixtures"
    CACHE PATH "Directory holding the extracted fixtures")

add_subdirectory(src/core)          # simulation: no SDL, no platform APIs
add_subdirectory(src/presentation)  # placeholder: vector/audio presentation state
add_subdirectory(src/input)         # placeholder: input adapters -> commands
add_subdirectory(src/platform)      # placeholder: SDL, storage, packaging
add_subdirectory(src/app)           # dcli today; desktop app later

if (DAGGORATH_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
