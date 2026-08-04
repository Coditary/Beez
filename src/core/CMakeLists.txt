add_library(beez_core STATIC context.cpp registry.cpp orchestrator.cpp phase_argument_parser.cpp plugin_host.cpp)

target_include_directories(beez_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_features(beez_core PUBLIC cxx_std_20)
