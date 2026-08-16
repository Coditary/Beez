if(NOT BEEZ_INCREMENT_LUA OR NOT BEEZ_INCREMENT_HEADER)
    message(FATAL_ERROR "embed_increment_lua.cmake requires BEEZ_INCREMENT_LUA and BEEZ_INCREMENT_HEADER")
endif()

file(READ "${BEEZ_INCREMENT_LUA}" BEEZ_INCREMENT_LUA_CONTENT)
file(WRITE "${BEEZ_INCREMENT_HEADER}" "#pragma once\n\nnamespace beez::plugin::lua {\ninline constexpr const char kIncrementRuntimeSource[] = R\"beez_increment(\n${BEEZ_INCREMENT_LUA_CONTENT})beez_increment\";\n}\n")
