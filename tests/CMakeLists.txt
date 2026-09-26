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

if (TARGET dod)
  add_test(NAME dod_trace_matches_dcli
    COMMAND bash -c "$<TARGET_FILE:dod> --headless --second 1 --jiffies 40 --script ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-7/traces/light-torch.script > ${CMAKE_BINARY_DIR}/dod-light.trace && $<TARGET_FILE:dcli> --second 1 --jiffies 40 --script ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-7/traces/light-torch.script --trace ${CMAKE_BINARY_DIR}/dcli-light.trace >/dev/null && diff -q ${CMAKE_BINARY_DIR}/dod-light.trace ${CMAKE_BINARY_DIR}/dcli-light.trace")
endif()
