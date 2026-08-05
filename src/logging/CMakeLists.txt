find_package(spdlog REQUIRED)

add_library(beez_logging STATIC recording_logger.cpp spdlog_backend.cpp)

target_include_directories(beez_logging PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_logging PUBLIC spdlog::spdlog)

target_compile_features(beez_logging PUBLIC cxx_std_20)
