find_package(CLI11 REQUIRED)
find_package(lua REQUIRED)

add_library(beez_cli STATIC cli_app.cpp list_formatter.cpp)

target_include_directories(beez_cli PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_cli PUBLIC CLI11::CLI11 beez_core lua::lua)

target_compile_features(beez_cli PUBLIC cxx_std_20)
