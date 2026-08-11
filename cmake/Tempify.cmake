include(FetchContent)

if(TARGET tempify_core)
    return()
endif()

if(NOT TARGET prebyte_core)
    message(FATAL_ERROR "Tempify integration requires prebyte_core. Include cmake/Prebyte.cmake first.")
endif()

if(NOT TARGET CLI11::CLI11)
    find_package(CLI11 REQUIRED)
endif()

set(TEMPIFY_VERSION "v0.1.2" CACHE STRING "Pinned Tempify git tag")
set(TEMPIFY_SOURCE_DIR "" CACHE PATH "Optional local Tempify source root override")

set(TEMPIFY_ROOT "")
if(TEMPIFY_SOURCE_DIR)
    get_filename_component(TEMPIFY_CANDIDATE "${TEMPIFY_SOURCE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
    if(EXISTS "${TEMPIFY_CANDIDATE}/src/main/include/tempify/app/TempifyApp.h")
        set(TEMPIFY_ROOT "${TEMPIFY_CANDIDATE}")
        message(STATUS "Using local Tempify source: ${TEMPIFY_ROOT}")
    else()
        message(WARNING
            "TEMPIFY_SOURCE_DIR='${TEMPIFY_SOURCE_DIR}' is missing or invalid. "
            "Fetching Tempify ${TEMPIFY_VERSION} from GitHub instead."
        )
    endif()
endif()

if(NOT TEMPIFY_ROOT)
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()

    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        message(FATAL_ERROR
            "Fetching Tempify from https://github.com/Coditary/Tempify.git at tag '${TEMPIFY_VERSION}' requires git. "
            "Install git or configure with -DTEMPIFY_SOURCE_DIR=/abs/path/to/Tempify."
        )
    endif()

    FetchContent_Declare(
        tempify
        GIT_REPOSITORY https://github.com/Coditary/Tempify.git
        GIT_TAG ${TEMPIFY_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    FetchContent_GetProperties(tempify)
    if(NOT tempify_POPULATED)
        message(STATUS "Fetching Tempify ${TEMPIFY_VERSION} from https://github.com/Coditary/Tempify.git")
        FetchContent_Populate(tempify)
    endif()

    set(TEMPIFY_ROOT "${tempify_SOURCE_DIR}")
endif()

file(GLOB_RECURSE TEMPIFY_CORE_SOURCES CONFIGURE_DEPENDS "${TEMPIFY_ROOT}/src/main/cpp/*.cpp")
list(FILTER TEMPIFY_CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(tempify_core STATIC ${TEMPIFY_CORE_SOURCES})
target_include_directories(tempify_core PUBLIC "${TEMPIFY_ROOT}/src/main/include")
target_link_libraries(tempify_core PUBLIC prebyte_core PRIVATE CLI11::CLI11)
target_compile_features(tempify_core PUBLIC cxx_std_23)

message(STATUS "Tempify core integrated from ${TEMPIFY_ROOT}")
