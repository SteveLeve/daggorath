add_executable(conformance_tests conformance/conformance_tests.cpp)
target_link_libraries(conformance_tests PRIVATE daggorath::core)
target_compile_definitions(conformance_tests
  PRIVATE DAG_FIXTURE_DIR="${DAGGORATH_FIXTURE_DIR}")
add_test(NAME conformance COMMAND conformance_tests)

add_executable(combat_regressions conformance/combat_regressions.cpp)
target_link_libraries(combat_regressions PRIVATE daggorath::core)
add_test(NAME combat_regressions COMMAND combat_regressions)

add_executable(object_regressions conformance/object_regressions.cpp)
target_link_libraries(object_regressions PRIVATE daggorath::core)
target_compile_definitions(object_regressions
  PRIVATE DAG_FIXTURE_DIR="${DAGGORATH_FIXTURE_DIR}")
add_test(NAME object_regressions COMMAND object_regressions)

add_executable(progression_regressions conformance/progression_regressions.cpp)
target_link_libraries(progression_regressions PRIVATE daggorath::core)
add_test(NAME progression_regressions COMMAND progression_regressions)

# Phase 3 fight traces: regenerate from the committed script and require identity.
set(PHASE3_TRACES ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-3/traces)
foreach(pair "fight-to-kill;300" "fight-to-death;1800")
  list(GET pair 0 name)
  list(GET pair 1 jiffies)
  add_test(NAME trace_${name}
    COMMAND sh -c "\"$<TARGET_FILE:dcli>\" --script \"${PHASE3_TRACES}/${name}.script\" --jiffies ${jiffies} --trace \"${CMAKE_CURRENT_BINARY_DIR}/${name}.trace\" && cmp \"${CMAKE_CURRENT_BINARY_DIR}/${name}.trace\" \"${PHASE3_TRACES}/${name}.trace\"")
endforeach()

# Identity check for the ROM trace diff. It does not claim a ROM was compared.
add_test(NAME trace_diff_identity
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/rom/trace_diff.py
          ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-0b/traces/t1-move-turn-look.trace
          ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-0b/traces/t1-move-turn-look.trace)

# Power-on to WINNER. Two dcli replays must emit WINNER and match byte for byte.
# The script contains D-12 FUDGE lines (not source behaviour).
set(PHASE5_TRACES ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-5/traces)
set(WINNER_SCRIPT ${PHASE5_TRACES}/power-on-to-winner.script)
add_test(NAME playthrough_power_on_to_winner
  COMMAND sh -c "\"$<TARGET_FILE:dcli>\" --script \"${WINNER_SCRIPT}\" --jiffies 230000 --trace \"${CMAKE_CURRENT_BINARY_DIR}/winner-a.trace\" && \"$<TARGET_FILE:dcli>\" --script \"${WINNER_SCRIPT}\" --jiffies 230000 --trace \"${CMAKE_CURRENT_BINARY_DIR}/winner-b.trace\" && grep -q WINNER \"${CMAKE_CURRENT_BINARY_DIR}/winner-a.trace\" && grep -q WINNER \"${CMAKE_CURRENT_BINARY_DIR}/winner-b.trace\" && cmp \"${CMAKE_CURRENT_BINARY_DIR}/winner-a.trace\" \"${CMAKE_CURRENT_BINARY_DIR}/winner-b.trace\"")
set_tests_properties(playthrough_power_on_to_winner PROPERTIES TIMEOUT 120)
