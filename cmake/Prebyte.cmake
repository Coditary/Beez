include(FetchContent)

if(TARGET prebyte_core)
    return()
endif()

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(
    prebyte
    GIT_REPOSITORY https://github.com/Coditary/Prebyte.git
    GIT_TAG v1.0.5
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(prebyte)
if(NOT prebyte_POPULATED)
    FetchContent_Populate(prebyte)

    file(GLOB PREBYTE_APP_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/app/*.cpp")
    file(GLOB PREBYTE_CLI_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/cli/*.cpp")
    file(GLOB PREBYTE_CONFIG_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/config/*.cpp")
    file(GLOB PREBYTE_IO_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/io/*.cpp")
    file(GLOB PREBYTE_RUNTIME_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/runtime/*.cpp")
    file(GLOB PREBYTE_SUPPORT_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/support/*.cpp")
    file(GLOB PREBYTE_TEMPLATE_AST_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/template/ast/*.cpp")
    file(GLOB PREBYTE_TEMPLATE_LEXER_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/template/lexer/*.cpp")
    file(GLOB PREBYTE_TEMPLATE_PARSER_SOURCES CONFIGURE_DEPENDS "${prebyte_SOURCE_DIR}/src/main/cpp/template/parser/*.cpp")

    set(PREBYTE_CORE_SOURCES
        ${PREBYTE_APP_SOURCES}
        ${PREBYTE_CLI_SOURCES}
        ${PREBYTE_CONFIG_SOURCES}
        ${PREBYTE_IO_SOURCES}
        ${PREBYTE_RUNTIME_SOURCES}
        ${PREBYTE_SUPPORT_SOURCES}
        ${PREBYTE_TEMPLATE_AST_SOURCES}
        ${PREBYTE_TEMPLATE_LEXER_SOURCES}
        ${PREBYTE_TEMPLATE_PARSER_SOURCES}
        "${prebyte_SOURCE_DIR}/src/main/cpp/PrebyteEngine.cpp"
        "${prebyte_SOURCE_DIR}/src/main/cpp/datatypes/Data.cpp"
        "${prebyte_SOURCE_DIR}/src/main/cpp/parser/EnvParser.cpp"
        "${prebyte_SOURCE_DIR}/src/main/cpp/parser/FileParser.cpp"
        "${prebyte_SOURCE_DIR}/src/main/cpp/parser/IniParser.cpp"
        "${prebyte_SOURCE_DIR}/src/main/cpp/parser/JsonParser.cpp"
        "${prebyte_SOURCE_DIR}/src/main/cpp/parser/TomlParser.cpp"
        "${prebyte_SOURCE_DIR}/src/main/cpp/parser/YamlParser.cpp"
    )

    add_library(prebyte_core STATIC ${PREBYTE_CORE_SOURCES})
    set_target_properties(prebyte_core PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

    target_include_directories(prebyte_core PUBLIC "${prebyte_SOURCE_DIR}/src/main/include")

    if(TARGET lua::lua)
        target_link_libraries(prebyte_core PUBLIC lua::lua)
        get_target_property(PREBYTE_LUA_INCLUDES lua::lua INTERFACE_INCLUDE_DIRECTORIES)
        if(PREBYTE_LUA_INCLUDES)
            target_include_directories(prebyte_core PUBLIC ${PREBYTE_LUA_INCLUDES})
        endif()
    elseif(TARGET Lua::Lua)
        target_link_libraries(prebyte_core PUBLIC Lua::Lua)
    else()
        find_package(Lua REQUIRED)
        target_include_directories(prebyte_core PUBLIC ${LUA_INCLUDE_DIR})
        target_link_libraries(prebyte_core PUBLIC ${LUA_LIBRARIES})
    endif()

    if(UNIX AND NOT APPLE)
        target_link_libraries(prebyte_core PUBLIC m)
    endif()
    if(CMAKE_DL_LIBS)
        target_link_libraries(prebyte_core PUBLIC ${CMAKE_DL_LIBS})
    endif()
endif()
