add_library(beez_plugin_host STATIC plugin_host.cpp)

target_include_directories(beez_plugin_host PUBLIC ${CMAKE_SOURCE_DIR}/include)
