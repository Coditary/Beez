find_package(CLI11 REQUIRED)
find_package(lua REQUIRED)
find_package(
    Python3
    COMPONENTS Interpreter
    REQUIRED
)

set(COMPLETION_EMBEDDED_CPP "${CMAKE_CURRENT_BINARY_DIR}/completion_embedded.cpp")
set(COMPLETION_SOURCE_DIR "${CMAKE_SOURCE_DIR}/scripts/completions")

add_custom_command(
    OUTPUT "${COMPLETION_EMBEDDED_CPP}"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/generate-completion-embedded.py"
            "${COMPLETION_SOURCE_DIR}" "${COMPLETION_EMBEDDED_CPP}"
    COMMAND clang-format -i "${COMPLETION_EMBEDDED_CPP}"
    DEPENDS "${CMAKE_SOURCE_DIR}/scripts/generate-completion-embedded.py" "${COMPLETION_SOURCE_DIR}/beez.bash"
            "${COMPLETION_SOURCE_DIR}/beez.zsh" "${COMPLETION_SOURCE_DIR}/activate.bash"
            "${COMPLETION_SOURCE_DIR}/activate.zsh" "${COMPLETION_SOURCE_DIR}/install-lib.sh"
    VERBATIM
)

add_library(
    beez_cli STATIC cli_app.cpp install_completion.cpp list_formatter.cpp name_suggestion.cpp run_target.cpp
                    "${COMPLETION_EMBEDDED_CPP}"
)

target_include_directories(beez_cli PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(beez_cli PUBLIC CLI11::CLI11 beez_core lua::lua)

target_compile_features(beez_cli PUBLIC cxx_std_20)
