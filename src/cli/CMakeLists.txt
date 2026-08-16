find_package(CLI11 REQUIRED)
find_package(lua REQUIRED)
find_package(
    Python3
    COMPONENTS Interpreter
    REQUIRED
)

set(COMPLETION_EMBEDDED_CPP "${CMAKE_CURRENT_BINARY_DIR}/completion_embedded.cpp")
set(COMPLETION_SOURCE_DIR "${CMAKE_SOURCE_DIR}/scripts/completions")

find_program(BEEZ_CLANG_FORMAT NAMES clang-format clang-format.exe)
if(BEEZ_CLANG_FORMAT)
    set(BEEZ_COMPLETION_POST_GEN COMMAND ${BEEZ_CLANG_FORMAT} -i "${COMPLETION_EMBEDDED_CPP}")
else()
    set(BEEZ_COMPLETION_POST_GEN COMMAND ${CMAKE_COMMAND} -E true)
endif()

add_custom_command(
    OUTPUT "${COMPLETION_EMBEDDED_CPP}"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/generate-completion-embedded.py"
            "${COMPLETION_SOURCE_DIR}" "${COMPLETION_EMBEDDED_CPP}"
    ${BEEZ_COMPLETION_POST_GEN}
    DEPENDS "${CMAKE_SOURCE_DIR}/scripts/generate-completion-embedded.py" "${COMPLETION_SOURCE_DIR}/beez.bash"
            "${COMPLETION_SOURCE_DIR}/beez.zsh" "${COMPLETION_SOURCE_DIR}/activate.bash"
            "${COMPLETION_SOURCE_DIR}/activate.zsh" "${COMPLETION_SOURCE_DIR}/install-lib.sh"
    VERBATIM
)

add_library(
    beez_cli STATIC
    parsing/cli_parser.cpp
    parsing/help_text.cpp
    parsing/phase_argument.cpp
    commands/run.cpp
    commands/list.cpp
    commands/config.cpp
    commands/cache.cpp
    commands/reqpack.cpp
    presentation/entity_table.cpp
    presentation/name_suggestion.cpp
    completion/install_completion.cpp
    session.cpp
    runner.cpp
    tempify_dispatch.cpp
    "${COMPLETION_EMBEDDED_CPP}"
)

target_include_directories(beez_cli PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_cli PUBLIC CLI11::CLI11 beez_core lua::lua tempify_core)

target_compile_features(beez_cli PUBLIC cxx_std_20)
