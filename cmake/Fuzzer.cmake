function(beez_enable_fuzzer target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Fuzzer + UBSan only: ASan/LSan is intentionally omitted here because
        # Lua/sol2 use longjmp and report false-positive leaks at process exit.
        # Memory safety is covered separately via `make sanitize`.
        target_compile_options(${target} PRIVATE
            -fsanitize=fuzzer,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE
            -fsanitize=fuzzer,undefined
        )
    else()
        message(WARNING "libFuzzer requires Clang")
    endif()
endfunction()
