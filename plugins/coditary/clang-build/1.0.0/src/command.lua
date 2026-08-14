local M = {}

local COMPILER_PATTERN = "^(/usr/bin/)?(llvm-)?(clang|clang\\+\\+)(%+%+)?"

function M.replace_compiler(command, cxx, cc)
    local first = command:match("^(%S+)")
    if first == nil then
        return command
    end

    if first:find("clang++", 1, true) or first:find("g++", 1, true) then
        return cxx .. command:sub(#first + 1)
    end

    if first:find("clang", 1, true) or first:find("gcc", 1, true) then
        return cc .. command:sub(#first + 1)
    end

    return command
end

function M.compile(config, entry)
    return "bash " .. beez.char.quote(entry.script)
end

function M.link(config, link_entry)
    return "bash " .. beez.char.quote(link_entry.script)
end

function M.index(config, root)
    local script = root .. "/" .. config.index_script
    local parts = {
        config.python_binary,
        beez.char.quote(script),
        beez.char.quote(config.build_tree_abs),
        beez.char.quote(config.build_tree_abs .. "/.beez-clang-index.lua"),
    }

    return table.concat(parts, " ")
end

return M
