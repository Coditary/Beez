find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
find_program(CPPCHECK_EXE NAMES "cppcheck")
find_program(IWYU_EXE NAMES "include-what-you-use")

if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY
        "${CLANG_TIDY_EXE};--use-color;--header-filter=(src|include|tests)/.*")
else()
    message(STATUS "clang-tidy not found")
endif()

if(CPPCHECK_EXE)
    message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
else()
    message(STATUS "cppcheck not found")
endif()

if(IWYU_EXE)
    message(STATUS "Found include-what-you-use: ${IWYU_EXE}")
else()
    message(STATUS "include-what-you-use not found")
endif()
