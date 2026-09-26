# The desktop window is built only when SDL3 is installed. The core and the
# offscreen rasterizer do not link it.
find_package(SDL3 QUIET)
if (SDL3_FOUND)
  add_executable(dod sdl_app.cpp)
  target_link_libraries(dod PRIVATE SDL3::SDL3 daggorath::core daggorath::presentation)
else()
  message(STATUS "SDL3 not found; desktop window target skipped")
endif()

add_library(daggorath_platform INTERFACE)
add_library(daggorath::platform ALIAS daggorath_platform)
