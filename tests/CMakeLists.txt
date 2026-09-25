add_executable(conformance_tests conformance/conformance_tests.cpp)
target_link_libraries(conformance_tests PRIVATE daggorath::core)
target_compile_definitions(conformance_tests
  PRIVATE DAG_FIXTURE_DIR="${DAGGORATH_FIXTURE_DIR}")
add_test(NAME conformance COMMAND conformance_tests)

# Phase 1 adds a second target here for creature population and regeneration,
# and a trace-diff test that replays docs/archaeology/phase-0b/traces/*.script
# against stored baselines.
