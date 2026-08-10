find_package(GTest REQUIRED)
find_package(CLI11 REQUIRED)
find_package(TBB REQUIRED)

add_executable(
    beez_tests
    version/test_version.cpp
    logging/backends/test_recording_logger.cpp
    logging/backends/test_spdlog_logger.cpp
    logging/console/test_output_mode.cpp
    logging/settings/test_logging_settings.cpp
    logging/console/test_progress_spinner.cpp
    logging/console/test_worker_output_format.cpp
    logging/persistence/test_run_log_writer.cpp
    cli/parsing/test_cli_parser.cpp
    cli/parsing/test_phase_argument.cpp
    cli/presentation/test_entity_table.cpp
    cli/presentation/test_name_suggestion.cpp
    cli/commands/test_run.cpp
    core/registry/test_registry.cpp
    core/config/cache/test_cache_options.cpp
    core/cache/fingerprint/test_content_hash.cpp
    core/cache/fingerprint/test_include_fingerprint.cpp
    core/cache/storage/test_cache_write_coordinator.cpp
    core/config/paths/test_config_paths.cpp
    core/test_temp_directory.cpp
    core/test_text_table.cpp
    core/cache/storage/test_cache_compress.cpp
    core/cache/storage/test_cache_storage.cpp
    core/glob/test_expand.cpp
    core/glob/test_metadata_cache.cpp
    core/cache/step/test_step_cache.cpp
    core/cache/success/test_success_cache.cpp
    core/execution/concurrency/test_thread_pool.cpp
    core/execution/concurrency/test_worker_pool.cpp
    core/env/test_env_file.cpp
    core/glob/test_pattern.cpp
    core/registry/test_step_order.cpp
    core/test_workflow.cpp
    core/runtime/test_context.cpp
    core/config/settings/test_settings.cpp
    core/config/report/test_settings_report.cpp
    core/orchestrator/test_orchestrator.cpp
    core/execution/process/test_stream_capture.cpp
    core/config/ui/test_progress_detail.cpp
    core/config/ui/test_ui_options.cpp
    core/config/performance/test_performance_options.cpp
    core/config/schema/test_config_schema.cpp
    plugin/host/test_plugin_host.cpp
    plugin/lua/test_lua_dsl.cpp
    plugin/lua/test_lua_dsl_fields.cpp
    plugin/lua/test_lua_settings.cpp
    plugin/lua/api/test_fs.cpp
    plugin/lua/api/test_crypto.cpp
    plugin/lua/api/sys/test_sys.cpp
    plugin/lua/api/time/test_time.cpp
    plugin/lua/api/date/test_date.cpp
    plugin/lua/api/data/test_data.cpp
    plugin/lua/api/net/test_net.cpp
    plugin/shell/test_shell_executor.cpp
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
