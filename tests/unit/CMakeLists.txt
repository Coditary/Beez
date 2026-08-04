find_package(GTest REQUIRED)

add_executable(
    beez_tests
    version/test_version.cpp
    core/test_registry.cpp
    core/test_workflow.cpp
    core/test_context.cpp
    core/test_orchestrator.cpp
    core/test_phase_argument_parser.cpp
    core/test_plugin_host.cpp
    plugins/lua/test_lua_dsl.cpp
    plugins/shell/test_shell_executor.cpp
)

target_link_libraries(beez_tests PRIVATE GTest::gtest GTest::gtest_main beez_core beez_plugins)

target_include_directories(beez_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(beez_tests)

if (BUILD_COVERAGE)
    beez_enable_coverage(beez_tests)
endif ()
