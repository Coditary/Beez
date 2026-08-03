add_library(beez_plugin_lua STATIC lua_dsl.cpp)

target_include_directories(beez_plugin_lua PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_plugin_lua PUBLIC beez_core sol2::sol2 lua::lua)
