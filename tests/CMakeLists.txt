if (BUILD_TESTING)
    add_subdirectory(unit)
    add_subdirectory(integration)
    add_subdirectory(system)
endif ()

add_subdirectory(fuzz)
