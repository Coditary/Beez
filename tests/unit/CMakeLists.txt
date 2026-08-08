find_package(GTest REQUIRED)
find_package(CLI11 REQUIRED)
find_package(TBB REQUIRED)

add_executable(
    beez_tests
    version/test_version.cpp
    logging/test_logger.cpp
    logging/test_worker_output_format.cpp
    cli/test_cli_app.cpp
    cli/test_list_formatter.cpp
    cli/test_name_suggestion.cpp
    cli/test_run_target.cpp
    core/test_registry.cpp
    core/test_content_hash.cpp
    core/test_include_fingerprint.cpp
    core/test_cache_options.cpp
    core/test_config_schema.cpp
    core/test_text_table.cpp
    core/test_cache_compress.cpp
    core/test_cache_storage.cpp
    core/test_glob_expand.cpp
    core/test_step_cache.cpp
    core/test_success_cache.cpp
    core/test_thread_pool.cpp
    core/test_worker_pool.cpp
    core/test_env_file.cpp
    core/test_glob_pattern.cpp
    core/test_step_order.cpp
    core/test_workflow.cpp
    core/test_context.cpp
    core/test_settings.cpp
    core/test_settings_report.cpp
    core/test_orchestrator.cpp
    core/test_stream_capture.cpp
    core/test_progress_detail.cpp
    core/test_phase_argument_parser.cpp
    core/test_plugin_host.cpp
    plugins/lua/test_lua_dsl.cpp
    plugins/lua/test_lua_settings.cpp
    plugins/shell/test_shell_executor.cpp
)

target_link_libraries(
    beez_tests
    PRIVATE GTest::gtest
            GTest::gtest_main
            beez_core
            beez_plugins
            beez_cli
            beez_logging
            CLI11::CLI11
)

target_include_directories(beez_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(beez_tests)

if (BUILD_COVERAGE)
    beez_enable_coverage(beez_tests)
endif ()
