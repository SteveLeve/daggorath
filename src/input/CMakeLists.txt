add_library(daggorath_input STATIC gesture.cpp)
add_library(daggorath::input ALIAS daggorath_input)
target_include_directories(daggorath_input PUBLIC include)
target_link_libraries(daggorath_input PUBLIC daggorath::core)
