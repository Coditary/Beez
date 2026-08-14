local defaults = require("src.defaults")

local runner = require("src.runner")

local function resolve_profile_build_tree(profile_name)
    local profile = defaults.build_profiles[profile_name]
    if profile.build_tree ~= nil then
        return profile.build_tree
    end

    if profile.build_tree_from_build_type then
        local build_type = beez.env_or("BUILD_TYPE", "Release")
        return "build/build/" .. build_type
    end

    return defaults.debug_build_tree
end

local function configure_step_def(step_name, profile_name)
    local profile = defaults.build_profiles[profile_name]
    local build_tree = resolve_profile_build_tree(profile_name)
    local build_type = profile.build_type or beez.env_or("BUILD_TYPE", "Release")

    local configure_outputs = profile.configure_outputs
    if type(configure_outputs) == "function" then
        configure_outputs = configure_outputs(build_tree)
    end

    local description = profile.configure_description
    if type(description) == "function" then
        description = description(build_type)
    end

    return {
        phase = "setup",
        scope = profile.scope,
        input = profile.configure_inputs,
        output = configure_outputs,
        description = description,
        config = { profile = profile_name },
        run = function(ctx)
            return runner.configure(ctx, step_name)
        end,
    }
end

local plugin_steps = {
    conan_graph_export = {
        phase = "package",
        scope = "audit",
        input = defaults.input_patterns,
        output = { defaults.graph_json },
        description = "Export Conan dependency graph (JSON)",
        config = {},
        run = function(ctx)
            return runner.graph_export(ctx)
        end,
    },

    conan_lock_create = {
        phase = "package",
        scope = "audit",
        input = defaults.input_patterns,
        output = { defaults.lockfile },
        description = "Create Conan lockfile",
        config = {},
        run = function(ctx)
            return runner.lock_create(ctx)
        end,
    },

    conan_sbom_export = {
        phase = "package",
        scope = "audit",
        input = defaults.input_patterns,
        output = {
            defaults.graph_json,
            defaults.cyclonedx_json,
        },
        description = "Export Conan graph and CycloneDX SBOM",
        config = {},
        run = function(ctx)
            return runner.sbom_export(ctx)
        end,
    },

    conan_install = {
        phase = "setup",
        scope = "app",
        input = defaults.input_patterns,
        output = { defaults.conan_output_folder .. "//**" },
        description = "Conan install only (no CMake configure)",
        config = { profile = "code" },
        run = function(ctx)
            return runner.install(ctx)
        end,
    },
}

for step_name, profile_name in pairs(defaults.configure_step_profiles) do
    plugin_steps[step_name] = configure_step_def(step_name, profile_name)
end

plugin("conan", {
    version = "1.0.0",
    description = "Conan install, CMake configure, graph export, lockfile, CycloneDX SBOM",
    organization = "coditary",

    config = {
        defaults = {
            conanfile = defaults.conanfile,
            conan_binary = defaults.conan_binary,
            python_binary = defaults.python_binary,
            cmake_binary = defaults.cmake_binary,
            conan_output_folder = defaults.conan_output_folder,
            build_policy = defaults.build_policy,
            converter_script = defaults.converter_script,
            profile_script = defaults.profile_script,
            sbom_dir = defaults.sbom_dir,
            graph_json = defaults.graph_json,
            cyclonedx_json = defaults.cyclonedx_json,
            lockfile = defaults.lockfile,
            input_patterns = defaults.input_patterns,
            log_prefix_graph = defaults.log_prefix_graph,
            log_prefix_lock = defaults.log_prefix_lock,
            log_prefix_sbom = defaults.log_prefix_sbom,
            log_prefix_install = defaults.log_prefix_install,
            log_prefix_configure = defaults.log_prefix_configure,
            log_prefix_build = defaults.log_prefix_build,
            graph_rev = defaults.graph_rev,
            lock_rev = defaults.lock_rev,
            sbom_rev = defaults.sbom_rev,
            install_rev = defaults.install_rev,
            configure_rev = defaults.configure_rev,
            build_rev = defaults.build_rev,
        },
    },

    steps = plugin_steps,
})
