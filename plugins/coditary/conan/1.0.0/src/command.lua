local shell = require("src.shell")

local M = {}

local function profile_pr_args()
    return "-pr \"$CONAN_PROFILE\" -pr:b \"$CONAN_PROFILE\""
end

function M.install(config, root)
    local parts = {
        config.profile_shell,
        "&&",
        config.conan_binary,
        "install",
        shell.quote(config.conanfile),
        "--output-folder=" .. shell.quote(config.conan_output_folder),
        "--build=" .. config.build_policy,
        "-s build_type=" .. config.build_type,
        profile_pr_args(),
    }

    return table.concat(parts, " ")
end

local function cmake_args_line(config, args)
    local parts = {
        config.cmake_binary,
        "--preset",
        config.cmake_preset,
    }

    for _, argument in ipairs(args) do
        parts[#parts + 1] = argument
    end

    return table.concat(parts, " ")
end

function M.cmake_configure(config)
    local parts = { cmake_args_line(config, config.cmake_first_args) }

    if config.cmake_second_args ~= nil and #config.cmake_second_args > 0 then
        parts[#parts + 1] = "&&"
        parts[#parts + 1] = cmake_args_line(config, config.cmake_second_args)
    end

    if config.post_configure ~= nil and config.post_configure ~= "" then
        parts[#parts + 1] = "&&"
        parts[#parts + 1] = config.post_configure
    end

    return table.concat(parts, " ")
end

function M.configure(config, root)
    local parts = {
        M.install(config, root),
        "&&",
        M.cmake_configure(config),
    }

    return table.concat(parts, " ")
end

function M.build(config)
    local parts = {
        config.cmake_binary,
        "--build",
        "--preset",
        config.cmake_preset,
    }

    if config.build_target ~= nil and config.build_target ~= "" then
        parts[#parts + 1] = "--target"
        parts[#parts + 1] = config.build_target
    end

    return table.concat(parts, " ")
end

function M.graph_info(config, root)
    local graph_path = root .. "/" .. config.graph_json
    local parts = {
        config.profile_shell,
        "&&",
        config.conan_binary,
        "graph info",
        shell.quote(config.conanfile),
        profile_pr_args(),
        "--format=json",
        "--out-file=" .. shell.quote(graph_path),
    }

    return table.concat(parts, " ")
end

function M.lock_create(config, root)
    local lock_path = root .. "/" .. config.lockfile
    local parts = {
        config.profile_shell,
        "&&",
        config.conan_binary,
        "lock create",
        shell.quote(root .. "/" .. config.conanfile),
        profile_pr_args(),
        "--lockfile-out=" .. shell.quote(lock_path),
    }

    return table.concat(parts, " ")
end

function M.cyclonedx_convert(config, root)
    local graph_path = root .. "/" .. config.graph_json
    local cyclonedx_path = root .. "/" .. config.cyclonedx_json
    local converter = root .. "/" .. config.converter_script

    local parts = {
        config.python_binary,
        shell.quote(converter),
        shell.quote(graph_path),
        shell.quote(cyclonedx_path),
    }

    return table.concat(parts, " ")
end

function M.mkdir_sbom_dir(config, root)
    return "mkdir -p " .. shell.quote(root .. "/" .. config.sbom_dir)
end

return M
