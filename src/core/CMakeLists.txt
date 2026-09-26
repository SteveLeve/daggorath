# Daggorath simulation core.
#
# Dependency rule, enforced by review: this target links nothing. No SDL, no
# platform headers, no I/O beyond the standard library. It must stay headless and
# testable. See docs/architecture/module-boundaries.md.
add_library(daggorath_core
  maze.cpp
  scheduler.cpp
  parser.cpp
  population.cpp
  creature_move.cpp
  combat.cpp
  core_event.cpp
  game.cpp
)
target_include_directories(daggorath_core PUBLIC include)
add_library(daggorath::core ALIAS daggorath_core)
