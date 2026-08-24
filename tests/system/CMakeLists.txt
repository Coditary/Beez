find_package(GTest REQUIRED)

add_executable(
    beez_system_tests
    helpers/process_runner.cpp
    scenarios/test_tasks.cpp
    scenarios/test_phase_tasks.cpp
    scenarios/test_workflows.cpp
    scenarios/test_errors.cpp
    scenarios/test_cli_flags.cpp
    scenarios/test_cli_link.cpp
    scenarios/test_config_build_lua.cpp
    scenarios/test_cache_usecases.cpp
    scenarios/test_cache_adversarial.cpp
    scenarios/test_list_entities.cpp
    scenarios/test_robustness.cpp
    scenarios/test_negative_fixtures.cpp
    scenarios/test_dsl_field_matrix.cpp
)

add_dependencies(beez_system_tests beez)

target_link_libraries(beez_system_tests PRIVATE GTest::gtest GTest::gtest_main)

target_include_directories(beez_system_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/tests/helpers)

target_compile_definitions(
    beez_system_tests
    PRIVATE "BEEZ_EXECUTABLE=\"$<TARGET_FILE:beez>\""
            "BEEZ_SYSTEM_FIXTURES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures\""
            "BEEZ_FUZZ_CORPUS_DIR=\"${CMAKE_SOURCE_DIR}/tests/fuzz/corpus/lua_dsl\""
)

include(GoogleTest)
gtest_discover_tests(beez_system_tests PROPERTIES LABELS system)
