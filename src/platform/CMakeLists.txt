# overlay_bridge links only daggorath::input (which itself links only
# daggorath::core) — no SDL — so its tap/picker state machine builds and
# tests in every environment, per ADR-0009's architecture table
# (docs/adr/0009-layering-and-shell.md), which gives src/shell (and, by the
# same reasoning, this bridge) headless testability.
# Storage (saves and preferences, storage.hpp) is one API with two backends:
# files on the desktop, which stays headless-testable here, and localStorage
# in the browser (ADR-0011).
add_library(daggorath_platform STATIC overlay_bridge.cpp prefs.cpp storage_common.cpp)
if (EMSCRIPTEN)
  target_sources(daggorath_platform PRIVATE web/web_storage.cpp)
else()
  target_sources(daggorath_platform PRIVATE file_storage.cpp)
endif()
add_library(daggorath::platform ALIAS daggorath_platform)
target_include_directories(daggorath_platform PUBLIC include)
target_link_libraries(daggorath_platform PUBLIC daggorath::input daggorath::presentation)

# The web build (ADR-0011) compiles the same window against Emscripten's SDL3
# port. ASYNCIFY lets its blocking loop and SDL_Delay calls yield to the
# browser; saves and preferences live in localStorage (web/web_storage.cpp,
# linked through daggorath_platform). The 8 MB stack covers the
# full-screen std::array frames the loop keeps as locals (the default is
# 64 KB). Output is dod.js + dod.wasm, loaded by web/index.html.
if (EMSCRIPTEN)
  add_executable(dod sdl_app.cpp)
  target_compile_options(dod PRIVATE --use-port=sdl3)
  target_link_options(dod PRIVATE --use-port=sdl3 -sASYNCIFY -sALLOW_MEMORY_GROWTH
                      -sENVIRONMENT=web -sSTACK_SIZE=8MB -sEXIT_RUNTIME -sMODULARIZE=0)
  target_link_libraries(dod PRIVATE daggorath::core daggorath::presentation
                                     daggorath::platform daggorath::shell)
  return()
endif()

# The Android build (Phase 9, ADR-0012) compiles the same window as
# libmain.so, the name SDL's Java SDLActivity loads, against SDL3 built from
# the pinned source tree tools/android/fetch-sdl.sh puts in third_party/
# (never committed). Saves live under SDL_GetPrefPath, which on Android is
# the app's private internal storage.
if (ANDROID)
  set(DAGGORATH_SDL3_SOURCE "${PROJECT_SOURCE_DIR}/third_party/SDL3" CACHE PATH
      "SDL3 source tree for the Android build (tools/android/fetch-sdl.sh)")
  if (NOT EXISTS "${DAGGORATH_SDL3_SOURCE}/CMakeLists.txt")
    message(FATAL_ERROR "SDL3 source not found at ${DAGGORATH_SDL3_SOURCE}; run tools/android/fetch-sdl.sh")
  endif()
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  add_subdirectory("${DAGGORATH_SDL3_SOURCE}" "${CMAKE_BINARY_DIR}/SDL3" EXCLUDE_FROM_ALL)
  add_library(main SHARED sdl_app.cpp)
  target_link_libraries(main PRIVATE SDL3::SDL3 daggorath::core daggorath::presentation
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
