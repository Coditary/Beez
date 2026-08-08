find_package(TBB REQUIRED)
find_package(ZLIB REQUIRED)

add_library(
    beez_core STATIC
    cache_compress.cpp
    cache_options.cpp
    cache_storage.cpp
    config_paths.cpp
    context.cpp
    content_hash.cpp
    include_fingerprint.cpp
    glob_expand.cpp
    glob_pattern.cpp
    registry.cpp
    orchestrator.cpp
    phase_argument_parser.cpp
    plugin_host.cpp
    settings.cpp
    settings_report.cpp
    step_cache.cpp
    success_cache.cpp
    thread_pool.cpp
    worker_pool.cpp
    step_config.cpp
    env_file.cpp
    step_order.cpp
    stream_capture.cpp
    progress_detail.cpp
)

target_include_directories(beez_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_core PUBLIC beez_logging TBB::tbb ZLIB::ZLIB)

target_compile_features(beez_core PUBLIC cxx_std_20)
