cmake_minimum_required(VERSION 3.24)

project(
    Beez
    VERSION 1.0.4
    DESCRIPTION "Beez - A modern C++ project"
    HOMEPAGE_URL "https://github.com/Leodoras/Beez"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

foreach (_shared_cpp_candidate "${CMAKE_SOURCE_DIR}/../../shared/Shared-Cpp"
                               "${CMAKE_SOURCE_DIR}/../../../shared/Shared-Cpp"
)
    if (EXISTS "${_shared_cpp_candidate}/cmake/CoditarySharedCpp.cmake")
        include("${_shared_cpp_candidate}/cmake/CoditarySharedCpp.cmake")
        break()
    endif ()
endforeach ()

if (NOT CODITARY_SHARED_CPP_DIR)
    message(FATAL_ERROR "Shared-Cpp not found next to the Beez checkout")
endif ()

include(CoditaryResolveUtils)
include(CoditaryResolveFs)
include(ResolveCompletion)
include(CompilerWarnings)
include(Sanitizers)
include(BuildCache)
include(Coverage)

coditary_resolve_utils()
coditary_resolve_fs()

set(STDCPP_PARSER_DIR "${CODITARY_SHARED_CPP_DIR}/std-cpp-parser")

if (NOT EXISTS "${STDCPP_PARSER_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "std-cpp-parser not found at ${STDCPP_PARSER_DIR}")
endif ()

# Normal variables (not stale CACHE) so std-cpp-parser format toggles apply on CI.
set(STDCPP_PARSER_BUILD_TESTS OFF)
set(STDCPP_PARSER_ENABLE_ALL_BY_DEFAULT ON)
set(STDCPP_PARSER_ENABLE_ENV ON)
set(STDCPP_PARSER_ENABLE_JSON ON)
set(STDCPP_PARSER_ENABLE_YAML ON)
set(STDCPP_PARSER_ENABLE_TOML ON)

add_subdirectory("${STDCPP_PARSER_DIR}" "${CMAKE_BINARY_DIR}/_deps/std-cpp-parser")

if (NOT TARGET std_cpp_parser_core)
    message(FATAL_ERROR "std-cpp-parser did not define std_cpp_parser_core")
endif ()

get_target_property(_stdcpp_core_sources std_cpp_parser_core SOURCES)
if (NOT _stdcpp_core_sources OR NOT "${STDCPP_PARSER_DIR}/src/env.cpp" IN_LIST _stdcpp_core_sources)
    target_sources(std_cpp_parser_core PRIVATE "${STDCPP_PARSER_DIR}/src/env.cpp")
    target_compile_definitions(std_cpp_parser_core PUBLIC STDCPP_PARSER_ENABLE_ENV)
    if (TARGET std_cpp_parser_codec)
        target_compile_definitions(std_cpp_parser_codec PUBLIC STDCPP_PARSER_ENABLE_ENV)
    endif ()
endif ()
resolve_completion()

add_subdirectory(src)

if (BUILD_TESTING)
    enable_testing()
endif ()

add_subdirectory(tests)

beez_apply_coverage_targets()

message(STATUS "Beez v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
