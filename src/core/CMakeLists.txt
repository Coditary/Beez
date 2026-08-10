find_package(TBB REQUIRED)
find_package(ZLIB REQUIRED)

add_library(
    beez_core STATIC
    cache/fingerprint/content_hash.cpp
    cache/fingerprint/include_fingerprint.cpp
    cache/storage/compress.cpp
    cache/storage/envelope.cpp
    cache/storage/migration.cpp
    cache/storage/write_coordinator.cpp
    cache/step/filesystem_store.cpp
    cache/step/index.cpp
    cache/step/key_strategy.cpp
    cache/step/output_tracker.cpp
    cache/step/step_cache.cpp
    cache/step/step_fingerprint.cpp
    cache/success/success_cache.cpp
    config/cache/cache_options.cpp
    config/env/env_settings.cpp
    config/paths/config_paths.cpp
    config/performance/performance_options.cpp
    config/report/cache_rows.cpp
    config/report/detail/report_helpers.cpp
    config/report/env_rows.cpp
    config/report/performance_rows.cpp
    config/report/settings_report.cpp
    config/report/ui_rows.cpp
    config/schema/config_schema.cpp
    config/settings/settings.cpp
    config/ui/progress_detail.cpp
    config/ui/progress_format.cpp
    config/ui/resolve.cpp
    config/ui/run_summary.cpp
    config/ui/theme.cpp
    runtime/context.cpp
    env/env_file.cpp
    glob/expand.cpp
    glob/metadata_cache.cpp
    glob/pattern.cpp
    registry/registry.cpp
    registry/step_order.cpp
    orchestrator/orchestrator.cpp
    orchestrator/errors.cpp
    orchestrator/run/stats.cpp
    orchestrator/run/lifecycle.cpp
    orchestrator/run/cache_skip.cpp
    orchestrator/run/shell_execution.cpp
    orchestrator/run/step_execution.cpp
    orchestrator/runners/shell.cpp
    orchestrator/runners/task.cpp
    orchestrator/runners/step.cpp
    orchestrator/runners/step_callback.cpp
    orchestrator/runners/phase.cpp
    orchestrator/runners/workflow.cpp
    execution/concurrency/thread_pool.cpp
    execution/concurrency/worker_pool.cpp
    execution/process/stream_capture.cpp
    model/step_config.cpp
    util/text_table.cpp
    util/temp_directory.cpp
)

target_include_directories(beez_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(beez_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config/report)
target_include_directories(beez_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cache/step)
target_include_directories(beez_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cache/storage)
target_include_directories(beez_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/orchestrator)

target_link_libraries(beez_core PUBLIC beez_logging TBB::tbb ZLIB::ZLIB)

target_compile_features(beez_core PUBLIC cxx_std_20)
