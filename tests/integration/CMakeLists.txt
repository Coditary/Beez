find_package(GTest REQUIRED)

add_executable(
    beez_integration_tests
    helpers/process_runner.cpp
    core/test_orchestrator_pipeline.cpp
    core/test_step_cache_pipeline.cpp
    core/test_success_cache_pipeline.cpp
    core/test_cache_flag_pipeline.cpp
    plugins/test_lua_shell_pipeline.cpp
    plugins/test_parallel_lua_steps.cpp
    app/test_cli.cpp
    app/test_cli_init.cpp
    app/test_cli_flags.cpp
    app/test_config_runtime.cpp
    app/test_completion.cpp
    app/test_install_completion.cpp
    app/test_security_scripts.cpp
)

add_dependencies(beez_integration_tests beez)

target_link_libraries(beez_integration_tests PRIVATE GTest::gtest GTest::gtest_main beez_core beez_plugins)

target_include_directories(beez_integration_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(beez_integration_tests PRIVATE "BEEZ_EXECUTABLE=\"$<TARGET_FILE:beez>\"")

include(GoogleTest)
gtest_discover_tests(beez_integration_tests PROPERTIES LABELS "integration")
