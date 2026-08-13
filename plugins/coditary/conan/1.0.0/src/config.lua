local defaults = require("src.defaults")
local env = require("src.env")
local shell = require("src.shell")

local M = {}

local function copy_string_array(values, fallback)
    if values == nil then
        return fallback
    end

    if type(values) ~= "table" then
        error("conan array config fields must be tables of strings")
    end

    local copied = {}
    for _, entry in ipairs(values) do
        if type(entry) ~= "string" then
            error("conan array config entries must be strings")
        end
        copied[#copied + 1] = entry
    end

    return copied
end

function M.resolve_profile_shell(config, root)
    local script = shell.quote(root .. "/" .. config.profile_script)

    if config.conan_profile ~= nil and config.conan_profile ~= "" then
        return "CONAN_PROFILE=" .. shell.quote(config.conan_profile)
    end

    return "CONAN_PROFILE=\"${CONAN_PROFILE:-$(" .. "bash " .. script .. ")}\""
end

local function resolve_build_type(profile, step_cfg)
    if step_cfg.build_type ~= nil then
        return step_cfg.build_type
    end

    if profile.build_type ~= nil then
        return profile.build_type
    end

    if profile.build_type_env then
        return env.env_or("BUILD_TYPE", "Release")
    end

    return "Release"
end

local function resolve_build_tree(profile, build_type, step_cfg)
    if step_cfg.build_tree ~= nil then
        return step_cfg.build_tree
    end

    if profile.build_tree ~= nil then
        return profile.build_tree
    end

    if profile.build_tree_from_build_type then
        return "build/build/" .. build_type
    end

    return defaults.debug_build_tree
end

local function resolve_cmake_preset(profile, build_type, step_cfg)
    if step_cfg.cmake_preset ~= nil then
        return step_cfg.cmake_preset
    end

    if profile.cmake_preset ~= nil then
        return profile.cmake_preset
    end

    if profile.cmake_preset_from_build_type then
        if build_type == "Debug" then
            return "conan-debug"
        end
        return "conan-release"
    end

    return "conan-debug"
end

local function resolve_post_configure(profile, root)
    if profile.post_configure == nil then
        return nil
    end

    if type(profile.post_configure) == "function" then
        return profile.post_configure(root)
    end

    return profile.post_configure
end

function M.resolve_supply(config, root)
    local cfg = config or {}

    local resolved = {
        conanfile = cfg.conanfile or defaults.conanfile,
        conan_binary = cfg.conan_binary or defaults.conan_binary,
        python_binary = cfg.python_binary or defaults.python_binary,
        reports_dir = cfg.reports_dir or env.env_or("REPORTS_DIR", defaults.reports_dir),
        sbom_dir = cfg.sbom_dir or defaults.sbom_dir,
        graph_json = cfg.graph_json or defaults.graph_json,
        cyclonedx_json = cfg.cyclonedx_json or defaults.cyclonedx_json,
        lockfile = cfg.lockfile or defaults.lockfile,
        converter_script = cfg.converter_script or defaults.converter_script,
        profile_script = cfg.profile_script or defaults.profile_script,
        conan_profile = cfg.conan_profile or env.env_or("CONAN_PROFILE", nil),
        log_prefix_graph = cfg.log_prefix_graph or defaults.log_prefix_graph,
        log_prefix_lock = cfg.log_prefix_lock or defaults.log_prefix_lock,
        log_prefix_sbom = cfg.log_prefix_sbom or defaults.log_prefix_sbom,
        graph_rev = cfg.graph_rev or defaults.graph_rev,
        lock_rev = cfg.lock_rev or defaults.lock_rev,
        sbom_rev = cfg.sbom_rev or defaults.sbom_rev,
        input_patterns = copy_string_array(cfg.input_patterns, defaults.input_patterns),
    }

    resolved.profile_shell = M.resolve_profile_shell(resolved, root)
    return resolved
end

function M.resolve_build(step_cfg, root, profile_name)
    local cfg = step_cfg or {}
    local profile = defaults.build_profiles[profile_name]
    if profile == nil then
        error("unknown conan build profile: " .. tostring(profile_name))
    end

    local build_type = resolve_build_type(profile, cfg)
    local build_tree = resolve_build_tree(profile, build_type, cfg)
    local cmake_preset = resolve_cmake_preset(profile, build_type, cfg)

    local resolved = {
        profile_name = profile_name,
        scope = profile.scope,
        conanfile = cfg.conanfile or defaults.conanfile,
        conan_binary = cfg.conan_binary or defaults.conan_binary,
        cmake_binary = cfg.cmake_binary or defaults.cmake_binary,
        conan_output_folder = cfg.conan_output_folder or defaults.conan_output_folder,
        build_policy = cfg.build_policy or defaults.build_policy,
        profile_script = cfg.profile_script or defaults.profile_script,
        conan_profile = cfg.conan_profile or env.env_or("CONAN_PROFILE", nil),
        build_type = build_type,
        build_tree = build_tree,
        cmake_preset = cmake_preset,
        cmake_first_args = copy_string_array(cfg.cmake_first_args, profile.cmake_first_args or {}),
        cmake_second_args = copy_string_array(cfg.cmake_second_args, profile.cmake_second_args or {}),
        build_target = cfg.build_target or profile.build_target,
        post_configure = cfg.post_configure or resolve_post_configure(profile, root),
        log_prefix_install = cfg.log_prefix_install or defaults.log_prefix_install,
        log_prefix_configure = cfg.log_prefix_configure or defaults.log_prefix_configure,
        log_prefix_build = cfg.log_prefix_build or defaults.log_prefix_build,
        install_rev = cfg.install_rev or defaults.install_rev,
        configure_rev = cfg.configure_rev or defaults.configure_rev,
        build_rev = cfg.build_rev or defaults.build_rev,
    }

    resolved.profile_shell = M.resolve_profile_shell(resolved, root)
    return resolved
end

return M
