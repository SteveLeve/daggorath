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

add_executable(gesture_tests input/gesture_tests.cpp)
target_link_libraries(gesture_tests PRIVATE daggorath::input)
target_compile_definitions(gesture_tests
  PRIVATE DAG_PHASE8_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/docs/archaeology/phase-8/fixtures")
add_test(NAME gesture_tests COMMAND gesture_tests)

add_executable(shell_tests shell/shell_tests.cpp)
target_link_libraries(shell_tests PRIVATE daggorath::shell)
add_test(NAME shell_tests COMMAND shell_tests)

# Phase 8.3: the `crisp` draw list, regenerated from the same golden states
# Phase 6's viewer fixtures fix, must match the committed fixture byte for
# byte — proof `crisp` and `pixel` project the same logical draw list
# (ADR-0010 §2), and that the Phase 6/7 golden pixel images are unaffected by
# the `walk_segment`/`mark4_rows` refactor (checked by the unchanged
# `viewer_regressions`/`text_regressions` results, not by this test alone).
add_executable(crisp_fixture_gen presentation/crisp_fixture_gen.cpp)
target_link_libraries(crisp_fixture_gen PRIVATE daggorath::core daggorath::presentation)
set(CRISP_FIXTURE ${CMAKE_SOURCE_DIR}/docs/archaeology/phase-8/fixtures/crisp-segments.txt)
add_test(NAME crisp_segment_fixtures
  COMMAND sh -c "\"$<TARGET_FILE:crisp_fixture_gen>\" > \"${CMAKE_CURRENT_BINARY_DIR}/crisp-segments.txt\" && cmp \"${CMAKE_CURRENT_BINARY_DIR}/crisp-segments.txt\" \"${CRISP_FIXTURE}\"")

add_executable(touch_overlay_tests input/touch_overlay_tests.cpp)
target_link_libraries(touch_overlay_tests PRIVATE daggorath::input)
add_test(NAME touch_overlay_tests COMMAND touch_overlay_tests)

add_executable(replay_equivalence_tests input/replay_equivalence_tests.cpp)
target_link_libraries(replay_equivalence_tests PRIVATE daggorath::input)
target_compile_definitions(replay_equivalence_tests
  PRIVATE DAG_PHASE0B_TRACE_DIR="${CMAKE_SOURCE_DIR}/docs/archaeology/phase-0b/traces")
add_test(NAME replay_equivalence_tests COMMAND replay_equivalence_tests)

add_executable(overlay_bridge_tests platform/overlay_bridge_tests.cpp)
target_link_libraries(overlay_bridge_tests PRIVATE daggorath::platform daggorath::input)
add_test(NAME overlay_bridge_tests COMMAND overlay_bridge_tests)

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
# Disabled 2026-09-27 (phase-5 reconciliation): the committed route relied on
# the fire ring's charge wrap, which the ring-charge fix removed. Re-enable
# once dplan is reworked and the script re-authored.
set_tests_properties(playthrough_power_on_to_winner PROPERTIES TIMEOUT 120 DISABLED TRUE)
