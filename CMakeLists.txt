cmake_minimum_required(VERSION 3.24)

project(
    Beez
    VERSION 1.0.3
    DESCRIPTION "Beez - A modern C++ project"
    HOMEPAGE_URL "https://github.com/Leodoras/Beez"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CompilerWarnings)
include(Sanitizers)
include(BuildCache)
include(Coverage)

add_subdirectory(src)

if (BUILD_TESTING)
    enable_testing()
endif ()

add_subdirectory(tests)

beez_apply_coverage_targets()

message(STATUS "Beez v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
