find_package(TBB REQUIRED)
find_package(ZLIB REQUIRED)

add_library(
    beez_core STATIC
    cache_compress.cpp
    cache_options.cpp
    cache_storage.cpp
    cache_write_coordinator.cpp
    config_paths.cpp
    context.cpp
    content_hash.cpp
    glob_metadata_cache.cpp
    include_fingerprint.cpp
    glob_expand.cpp
    glob_pattern.cpp
    performance_options.cpp
    ui_options.cpp
    registry.cpp
    orchestrator.cpp
    phase_argument_parser.cpp
    plugin_host.cpp
    settings.cpp
    settings_report.cpp
    config_schema.cpp
    step_cache.cpp
    success_cache.cpp
    text_table.cpp
    thread_pool.cpp
    worker_pool.cpp
    step_config.cpp
    env_file.cpp
    env_settings.cpp
    step_order.cpp
    stream_capture.cpp
    progress_detail.cpp
    temp_directory.cpp
)

target_include_directories(beez_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_core PUBLIC beez_logging TBB::tbb ZLIB::ZLIB)

target_compile_features(beez_core PUBLIC cxx_std_20)
