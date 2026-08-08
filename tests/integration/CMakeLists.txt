find_package(GTest REQUIRED)

add_executable(
    beez_integration_tests
    helpers/process_runner.cpp core/test_orchestrator_pipeline.cpp core/test_step_cache_pipeline.cpp
    core/test_success_cache_pipeline.cpp plugins/test_lua_shell_pipeline.cpp app/test_cli.cpp
)

add_dependencies(beez_integration_tests beez)

target_link_libraries(beez_integration_tests PRIVATE GTest::gtest GTest::gtest_main beez_core beez_plugins)

target_include_directories(beez_integration_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(beez_integration_tests PRIVATE "BEEZ_EXECUTABLE=\"$<TARGET_FILE:beez>\"")

include(GoogleTest)
gtest_discover_tests(beez_integration_tests)

if (BUILD_COVERAGE)
    beez_enable_coverage(beez_integration_tests)
endif ()
