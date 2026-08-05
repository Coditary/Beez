add_library(
    beez_core STATIC
    context.cpp
    registry.cpp
    orchestrator.cpp
    phase_argument_parser.cpp
    plugin_host.cpp
    step_config.cpp
    stream_capture.cpp
    progress_detail.cpp
)

target_include_directories(beez_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_core PUBLIC beez_logging)

target_compile_features(beez_core PUBLIC cxx_std_20)
