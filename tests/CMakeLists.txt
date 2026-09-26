add_executable(conformance_tests conformance/conformance_tests.cpp)
target_link_libraries(conformance_tests PRIVATE daggorath::core daggorath::presentation)
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

add_executable(text_regressions conformance/text_regressions.cpp)
target_link_libraries(text_regressions PRIVATE daggorath::core daggorath::presentation)
target_compile_definitions(text_regressions
  PRIVATE DAG_FIXTURE_DIR="${DAGGORATH_FIXTURE_DIR}"
          DAG_TEXT_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/docs/archaeology/phase-6/fixtures/text")
add_test(NAME text_regressions COMMAND text_regressions)

add_executable(progression_regressions conformance/progression_regressions.cpp)
target_link_libraries(progression_regressions PRIVATE daggorath::core)
add_test(NAME progression_regressions COMMAND progression_regressions)

add_executable(viewer_regressions conformance/viewer_regressions.cpp)
target_link_libraries(viewer_regressions PRIVATE daggorath::core daggorath::presentation)
target_compile_definitions(viewer_regressions
  PRIVATE DAG_PHASE6_DIR="${DAGGORATH_PHASE6_DIR}")
add_test(NAME viewer_regressions COMMAND viewer_regressions)

add_executable(event_regressions conformance/event_regressions.cpp)
target_link_libraries(event_regressions PRIVATE daggorath::core)
target_compile_definitions(event_regressions
  PRIVATE DAG_FIXTURE_DIR="${DAGGORATH_FIXTURE_DIR}")
add_test(NAME event_regressions COMMAND event_regressions)

add_test(NAME port_creature_stats
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/compare_port_creatures.py)

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

if (TARGET dod)
  add_test(NAME dod_trace_matches_dcli
    COMMAND bash -c "$<TARGET_FILE:dod> --headless --second 1 --jiffies 40 --script ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-7/traces/light-torch.script > ${CMAKE_BINARY_DIR}/dod-light.trace && $<TARGET_FILE:dcli> --second 1 --jiffies 40 --script ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-7/traces/light-torch.script --trace ${CMAKE_BINARY_DIR}/dcli-light.trace >/dev/null && diff -q ${CMAKE_BINARY_DIR}/dod-light.trace ${CMAKE_BINARY_DIR}/dcli-light.trace")
  add_test(NAME dod_thud_samples
    COMMAND bash -c "$<TARGET_FILE:dod> --headless --second 1 --jiffies 40 --script ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-7/traces/blocked-move.script --samples ${CMAKE_BINARY_DIR}/thud.raw > ${CMAKE_BINARY_DIR}/dod-thud.trace && test $(wc -c < ${CMAKE_BINARY_DIR}/thud.raw) -eq 104")
endif()
# Power-on to WINNER. Two dcli replays must emit WINNER and match byte for byte.
# The script contains D-12 FUDGE lines (not source behaviour).
set(PHASE5_TRACES ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-5/traces)
set(WINNER_SCRIPT ${PHASE5_TRACES}/power-on-to-winner.script)
add_test(NAME playthrough_power_on_to_winner
  COMMAND sh -c "\"$<TARGET_FILE:dcli>\" --script \"${WINNER_SCRIPT}\" --jiffies 230000 --trace \"${CMAKE_CURRENT_BINARY_DIR}/winner-a.trace\" && \"$<TARGET_FILE:dcli>\" --script \"${WINNER_SCRIPT}\" --jiffies 230000 --trace \"${CMAKE_CURRENT_BINARY_DIR}/winner-b.trace\" && grep -q WINNER \"${CMAKE_CURRENT_BINARY_DIR}/winner-a.trace\" && grep -q WINNER \"${CMAKE_CURRENT_BINARY_DIR}/winner-b.trace\" && cmp \"${CMAKE_CURRENT_BINARY_DIR}/winner-a.trace\" \"${CMAKE_CURRENT_BINARY_DIR}/winner-b.trace\"")
set_tests_properties(playthrough_power_on_to_winner PROPERTIES TIMEOUT 120)
