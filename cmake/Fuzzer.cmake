function(beez_enable_fuzzer target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE
            -fsanitize=fuzzer,address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE
            -fsanitize=fuzzer,address,undefined
        )
    else()
        message(WARNING "libFuzzer requires Clang")
    endif()
endfunction()
