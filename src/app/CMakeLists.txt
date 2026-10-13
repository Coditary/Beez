add_executable(beez main.cpp)

target_link_libraries(beez PRIVATE beez_core beez_plugins beez_cli beez_logging)

set_target_properties(beez PROPERTIES OUTPUT_NAME "beez" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(CompilerWarnings)
beez_set_warnings(beez PRIVATE)
