# overlay_bridge links only daggorath::input (which itself links only
# daggorath::core) — no SDL — so its tap/picker state machine builds and
# tests in every environment, per ADR-0009's architecture table
# (docs/adr/0009-layering-and-shell.md), which gives src/shell (and, by the
# same reasoning, this bridge) headless testability.
add_library(daggorath_platform STATIC overlay_bridge.cpp)
add_library(daggorath::platform ALIAS daggorath_platform)
target_include_directories(daggorath_platform PUBLIC include)
target_link_libraries(daggorath_platform PUBLIC daggorath::input)

# The desktop window is built only when SDL3 is installed. The core, the
# offscreen rasterizer and the overlay bridge do not link it.
find_package(SDL3 QUIET)
if (SDL3_FOUND)
  add_executable(dod sdl_app.cpp)
  target_link_libraries(dod PRIVATE SDL3::SDL3 daggorath::core daggorath::presentation
                                     daggorath::platform daggorath::shell)
else()
  message(STATUS "SDL3 not found; desktop window target skipped")
endif()
