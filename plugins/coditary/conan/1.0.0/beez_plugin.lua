local defaults = require("src.defaults")
local step_config = require("src.step_config")
local env = require("src.env")

local runner = require("src.runner")

local function resolve_profile_build_tree(profile_name)
    local profile = defaults.build_profiles[profile_name]
    if profile.build_tree ~= nil then
        return profile.build_tree
    end

    if profile.build_tree_from_build_type then
        local build_type = env.env_or("BUILD_TYPE", "Release")
        return "build/build/" .. build_type
    end

    return defaults.debug_build_tree
end

local function configure_step_def(step_name, profile_name)
    local profile = defaults.build_profiles[profile_name]
    local build_tree = resolve_profile_build_tree(profile_name)
    local build_type = profile.build_type or env.env_or("BUILD_TYPE", "Release")

    local configure_outputs = profile.configure_outputs
    if type(configure_outputs) == "function" then
        configure_outputs = configure_outputs(build_tree)
    end

    local description = profile.configure_description
    if type(description) == "function" then
        description = description(build_type)
    end

    return {
        phase = "configure",
        scope = profile.scope,
        input = profile.configure_inputs,
        output = configure_outputs,
        description = description,
        config = step_config.configure_defaults(profile_name),
        run = function(ctx)
            return runner.configure(ctx, step_name)
        end,
    }
end

local plugin_steps = {
    conan_graph_export = {
        phase = "qa",
        scope = "supply-graph",
        input = defaults.input_patterns,
        output = { defaults.graph_json },
        description = "Export Conan dependency graph (JSON)",
        config = step_config.graph_defaults(),
        run = function(ctx)
            return runner.graph_export(ctx)
        end,
    },

    conan_lock_create = {
        phase = "qa",
        scope = "supply",
        input = defaults.input_patterns,
        output = { defaults.lockfile },
        description = "Create Conan lockfile",
        config = step_config.lock_defaults(),
        run = function(ctx)
            return runner.lock_create(ctx)
        end,
    },

    conan_sbom_export = {
        phase = "qa",
        scope = "supply",
        input = defaults.input_patterns,
        output = {
            defaults.graph_json,
            defaults.cyclonedx_json,
        },
        description = "Export Conan graph and CycloneDX SBOM",
        config = step_config.sbom_defaults(),
        run = function(ctx)
            return runner.sbom_export(ctx)
        end,
    },

    conan_install = {
        phase = "configure",
        scope = "conan",
        input = defaults.input_patterns,
        output = { defaults.conan_output_folder .. "//**" },
        description = "Conan install only (no CMake configure)",
        config = step_config.install_defaults(),
        run = function(ctx)
            return runner.install(ctx)
        end,
    },
}

for step_name, profile_name in pairs(defaults.configure_step_profiles) do
    plugin_steps[step_name] = configure_step_def(step_name, profile_name)
end

-- Beez Conan plugin
--
-- Steps:
--   conan_install         — conan install only
--   configure:setup       — conan install + cmake (Release/Debug via BUILD_TYPE)
--   configure:debug       — Debug toolchain configure
--   configure:coverage    — coverage configure
--   configure:sanitize    — ASan/UBSan configure
--   configure:tsan        — TSan configure
--   configure:fuzzer      — fuzzer configure
--   (compile/link via coditary/clang-build plugin)
--   conan_graph_export    — graph JSON only (scope supply-graph)
--   conan_lock_create     — lockfile
--   conan_sbom_export     — graph + CycloneDX
--
-- Env: BUILD_TYPE, CONAN_PROFILE, REPORTS_DIR (via beez.env / .env)
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/conan",
--             path = "./plugins/coditary/conan",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("conan", {
    version = "1.0.0",
    description = "Conan install, CMake configure, graph export, lockfile, CycloneDX SBOM",
    organization = "coditary",
    steps = plugin_steps,
})
