find_package(TBB REQUIRED)
find_package(ZLIB REQUIRED)

add_library(
    beez_core STATIC
    cache/compress.cpp
    cache/content_hash.cpp
    cache/filesystem_store.cpp
    cache/include_fingerprint.cpp
    cache/key_strategy.cpp
    cache/output_tracker.cpp
    cache/step_cache.cpp
    cache/step_cache_detail.cpp
    cache/storage.cpp
    cache/success_cache.cpp
    cache/write_coordinator.cpp
    config/cache_options.cpp
    config/config_paths.cpp
    config/config_schema.cpp
    config/env_settings.cpp
    config/performance_options.cpp
    config/report/cache_rows.cpp
    config/report/env_rows.cpp
    config/report/performance_rows.cpp
    config/report/report_helpers.cpp
    config/report/ui_rows.cpp
    config/settings.cpp
    config/settings_report.cpp
    config/ui/run_summary.cpp
    config/ui_options.cpp
    runtime/context.cpp
    env/env_file.cpp
    glob/expand.cpp
    glob/metadata_cache.cpp
    glob/pattern.cpp
    registry/registry.cpp
    registry/step_order.cpp
    orchestrator/orchestrator.cpp
    orchestrator/orchestrator_detail.cpp
    orchestrator/run_stats.cpp
    orchestrator/shell_runner.cpp
    orchestrator/task_runner.cpp
    orchestrator/step_runner.cpp
    orchestrator/phase_runner.cpp
    orchestrator/workflow_runner.cpp
    execution/phase_argument_parser.cpp
    execution/progress_detail.cpp
    execution/stream_capture.cpp
    execution/thread_pool.cpp
    execution/worker_pool.cpp
    model/step_config.cpp
    util/text_table.cpp
    util/temp_directory.cpp
)

target_include_directories(beez_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(beez_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config/report)
target_include_directories(beez_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cache)
target_include_directories(beez_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/orchestrator)

target_link_libraries(beez_core PUBLIC beez_logging TBB::tbb ZLIB::ZLIB)

target_compile_features(beez_core PUBLIC cxx_std_20)
