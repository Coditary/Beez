add_library(beez_plugin_lua STATIC lua_dsl.cpp lua_settings.cpp lua_step_config.cpp)

target_include_directories(beez_plugin_lua PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(beez_plugin_lua PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(beez_plugin_lua PUBLIC beez_core sol2::sol2 lua::lua)
