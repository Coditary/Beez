local command = require("src.command")

local M = {}

local function load_index(config, root, ctx)
    local index_path = config.build_tree_abs .. "/.beez-clang-index.lua"
    local compdb_path = config.build_tree_abs .. "/compile_commands.json"

    if not beez.fs.exists(compdb_path) then
        print(config.log_prefix_compile .. " compile_commands.json not found: " .. compdb_path)
        print(config.log_prefix_compile .. " Run configure first (e.g. beez -s configure:setup).")
        return nil, 2
    end

    local code = beez.shell.run(ctx, config.log_prefix_compile, command.index(config, root))
    if code ~= 0 then
        return nil, code
    end

    local chunk, load_error = loadfile(index_path)
    if chunk == nil then
        print(config.log_prefix_compile .. " failed to load index: " .. tostring(load_error))
        return nil, 1
    end

    local index = chunk()
    if index == nil or index.entries == nil then
        print(config.log_prefix_compile .. " invalid clang index at " .. index_path)
        return nil, 1
    end

    return index, 0
end

return {
    load_index = load_index,
}
