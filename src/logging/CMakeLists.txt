find_package(spdlog REQUIRED)

add_library(
    beez_logging STATIC
    backends/recording_logger.cpp
    backends/spdlog_setup.cpp
    backends/spdlog_logger.cpp
    console/progress_spinner.cpp
    console/worker_output_format.cpp
    console/output_mode.cpp
    settings/logging_settings.cpp
    persistence/run_log_writer.cpp
)

target_include_directories(beez_logging PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_logging PUBLIC spdlog::spdlog)

target_compile_features(beez_logging PUBLIC cxx_std_20)
