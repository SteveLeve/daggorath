# The system-menu shell (ADR-0009). Headless: links only daggorath::core, so
# pause/resume, snapshot slots and the pause-invariance test run without
# SDL3. See src/shell/include/daggorath/shell.hpp.
add_library(daggorath_shell STATIC shell.cpp system_menu.cpp)
add_library(daggorath::shell ALIAS daggorath_shell)
target_include_directories(daggorath_shell PUBLIC include)
target_link_libraries(daggorath_shell PUBLIC daggorath::core)
