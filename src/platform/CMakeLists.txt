# overlay_bridge links only daggorath::input (which itself links only
# daggorath::core) — no SDL — so its tap/picker state machine builds and
# tests in every environment, per ADR-0009's architecture table
# (docs/adr/0009-layering-and-shell.md), which gives src/shell (and, by the
# same reasoning, this bridge) headless testability.
add_library(daggorath_platform STATIC overlay_bridge.cpp)
add_library(daggorath::platform ALIAS daggorath_platform)
target_include_directories(daggorath_platform PUBLIC include)
target_link_libraries(daggorath_platform PUBLIC daggorath::input daggorath::presentation)

# The web build (ADR-0011) compiles the same window against Emscripten's SDL3
# port. ASYNCIFY lets its blocking loop and SDL_Delay calls yield to the
# browser; IDBFS backs the save directory. The 8 MB stack covers the
# full-screen std::array frames the loop keeps as locals (the default is
# 64 KB). Output is dod.js + dod.wasm, loaded by web/index.html.
if (EMSCRIPTEN)
  add_executable(dod sdl_app.cpp web/persist.cpp)
  target_compile_options(dod PRIVATE --use-port=sdl3)
  target_link_options(dod PRIVATE --use-port=sdl3 -sASYNCIFY -sALLOW_MEMORY_GROWTH
                      -sENVIRONMENT=web -sSTACK_SIZE=8MB -sEXIT_RUNTIME -sMODULARIZE=0 -lidbfs.js)
  target_link_libraries(dod PRIVATE daggorath::core daggorath::presentation
                                     daggorath::platform daggorath::shell)
  return()
endif()

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
