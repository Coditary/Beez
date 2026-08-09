find_package(GTest REQUIRED)

add_executable(beez_perf_tests test_artifact_vault_throughput.cpp)

target_link_libraries(beez_perf_tests PRIVATE GTest::gtest GTest::gtest_main beez_core beez_plugins)

target_include_directories(beez_perf_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(
    beez_perf_tests PRIVATE "BEEZ_PERF_ROOT_DIR=\"${CMAKE_BINARY_DIR}/test/performance\""
                            "BEEZ_PERF_REPORT_DIR=\"${CMAKE_BINARY_DIR}/test/performance/reports\""
)

include(GoogleTest)
gtest_discover_tests(
    beez_perf_tests
    PROPERTIES LABELS performance
    TIMEOUT 600
)
