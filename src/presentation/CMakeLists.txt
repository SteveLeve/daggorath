add_library(daggorath_presentation
  render_state.cpp
  mapper.cpp
  examine.cpp
  text.cpp)
target_include_directories(daggorath_presentation PUBLIC include)
target_link_libraries(daggorath_presentation PUBLIC daggorath::core)
add_library(daggorath::presentation ALIAS daggorath_presentation)
