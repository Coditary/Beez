if (BUILD_TESTING)
    add_subdirectory(unit)
    add_subdirectory(integration)
    add_subdirectory(system)
    add_subdirectory(performance)
endif ()

add_subdirectory(fuzz)
