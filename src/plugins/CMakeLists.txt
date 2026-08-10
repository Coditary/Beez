find_package(lua REQUIRED)
find_package(yyjson REQUIRED)
find_package(ryml REQUIRED)
find_package(rapidxml REQUIRED)
find_package(fast-cpp-csv-parser REQUIRED)
find_package(tomlplusplus REQUIRED)

include(FetchContent)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(sol2)

add_subdirectory(host)
add_subdirectory(lua)
add_subdirectory(shell)

add_library(beez_plugins INTERFACE)

target_link_libraries(beez_plugins INTERFACE beez_plugin_host beez_plugin_lua beez_plugin_shell sol2::sol2 lua::lua)

target_include_directories(beez_plugins INTERFACE ${CMAKE_SOURCE_DIR}/include)
