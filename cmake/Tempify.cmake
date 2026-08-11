if(TARGET tempify_core)
    return()
endif()

if(NOT TARGET prebyte_core)
    message(FATAL_ERROR "Tempify integration requires prebyte_core. Include cmake/Prebyte.cmake first.")
endif()

if(NOT TARGET CLI11::CLI11)
    find_package(CLI11 REQUIRED)
endif()

set(TEMPIFY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Tempify" CACHE PATH "Tempify source root")

if(NOT EXISTS "${TEMPIFY_SOURCE_DIR}/src/main/include/tempify/app/TempifyApp.h")
    message(FATAL_ERROR
        "TEMPIFY_SOURCE_DIR='${TEMPIFY_SOURCE_DIR}' is invalid. "
        "Expected Tempify source root with src/main/include/tempify/app/TempifyApp.h."
    )
endif()

file(GLOB_RECURSE TEMPIFY_CORE_SOURCES CONFIGURE_DEPENDS "${TEMPIFY_SOURCE_DIR}/src/main/cpp/*.cpp")
list(FILTER TEMPIFY_CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(tempify_core STATIC ${TEMPIFY_CORE_SOURCES})
target_include_directories(tempify_core PUBLIC "${TEMPIFY_SOURCE_DIR}/src/main/include")
target_link_libraries(tempify_core PUBLIC prebyte_core PRIVATE CLI11::CLI11)
target_compile_features(tempify_core PUBLIC cxx_std_23)

message(STATUS "Tempify core integrated from ${TEMPIFY_SOURCE_DIR}")
