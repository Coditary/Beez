find_package(GTest REQUIRED)

add_executable(
    beez_system_tests helpers/process_runner.cpp scenarios/test_tasks.cpp scenarios/test_phase_tasks.cpp
                      scenarios/test_workflows.cpp scenarios/test_errors.cpp
)

add_dependencies(beez_system_tests beez)

target_link_libraries(beez_system_tests PRIVATE GTest::gtest GTest::gtest_main)

target_include_directories(beez_system_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(
    beez_system_tests PRIVATE "BEEZ_EXECUTABLE=\"$<TARGET_FILE:beez>\""
                              "BEEZ_SYSTEM_FIXTURES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures\""
)

include(GoogleTest)
gtest_discover_tests(beez_system_tests PROPERTIES LABELS system)

if (BUILD_COVERAGE)
    beez_enable_coverage(beez_system_tests)
endif ()
