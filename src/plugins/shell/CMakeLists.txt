add_library(beez_plugin_shell STATIC shell_executor.cpp)

target_include_directories(beez_plugin_shell PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_plugin_shell PUBLIC beez_core)
