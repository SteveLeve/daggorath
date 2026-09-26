add_library(daggorath_presentation render_state.cpp raster.cpp snoise.cpp vctlst.cpp)
target_include_directories(daggorath_presentation PUBLIC include)
add_library(daggorath::presentation ALIAS daggorath_presentation)
