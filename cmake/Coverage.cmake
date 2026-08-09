function(beez_enable_coverage target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE --coverage)
        target_link_options(${target} PRIVATE --coverage)
    endif()
endfunction()

function(beez_link_coverage target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_link_options(${target} PRIVATE --coverage)
    endif()
endfunction()

# Instrument libraries and in-process unit tests only.
# Do not instrument the beez executable or subprocess-based test runners: spawning a
# coverage-instrumented binary from another instrumented process corrupts .gcda files.
function(beez_apply_coverage_targets)
    if(NOT BUILD_COVERAGE)
        return()
    endif()

    foreach(target IN ITEMS
            beez_core
            beez_logging
            beez_cli
            beez_plugin_lua
            beez_plugin_shell
            beez_tests
    )
        if(TARGET ${target})
            beez_enable_coverage(${target})
        endif()
    endforeach()

    foreach(target IN ITEMS beez beez_integration_tests beez_system_tests beez_perf_tests)
        if(TARGET ${target})
            beez_link_coverage(${target})
        endif()
    endforeach()
endfunction()
