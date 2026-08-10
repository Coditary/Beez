add_library(
    beez_plugin_lua STATIC
    lua_dsl.cpp
    api/beez_table.cpp
    api/env.cpp
    api/fs.cpp
    api/crypto.cpp
    api/detail/crypto_ops.cpp
    dsl/task_parser.cpp
    dsl/step_parser.cpp
    dsl/workflow_parser.cpp
    dsl/registry_validation.cpp
    dsl/dsl_binder.cpp
    settings/lua_settings.cpp
    settings/settings_overlay.cpp
    runtime/step_config.cpp
    runtime/worker_parser.cpp
)

target_include_directories(beez_plugin_lua PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_plugin_lua PUBLIC beez_core beez_plugin_host sol2::sol2 lua::lua)
