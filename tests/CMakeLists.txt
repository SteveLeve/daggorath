add_executable(conformance_tests conformance/conformance_tests.cpp)
target_link_libraries(conformance_tests PRIVATE daggorath::core daggorath::presentation)
target_compile_definitions(conformance_tests
  PRIVATE DAG_FIXTURE_DIR="${DAGGORATH_FIXTURE_DIR}")
add_test(NAME conformance COMMAND conformance_tests)

# Identity check for the ROM trace diff. It does not claim a ROM was compared.
add_test(NAME trace_diff_identity
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/rom/trace_diff.py
          ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-0b/traces/t1-move-turn-look.trace
          ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-0b/traces/t1-move-turn-look.trace)
