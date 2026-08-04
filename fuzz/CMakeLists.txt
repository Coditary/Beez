option(BUILD_FUZZER "Build fuzzer targets" OFF)

if (BUILD_FUZZER)
    include(Fuzzer)

    add_executable(fuzz_lua_dsl fuzz_lua_dsl.cpp)

    target_include_directories(fuzz_lua_dsl PRIVATE ${CMAKE_SOURCE_DIR}/include)

    target_link_libraries(fuzz_lua_dsl PRIVATE beez_plugins)

    beez_enable_fuzzer(fuzz_lua_dsl)

    set_target_properties(fuzz_lua_dsl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fuzz")
endif ()
