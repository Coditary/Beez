local defaults = require("src.defaults")
local step_config = require("src.step_config")

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

local function resolve_outputs(profile, build_tree)
    local outputs = profile.compile_outputs
    if type(outputs) == "function" then
        return outputs(build_tree)
    end

    return outputs
end

local function compile_step_def(step_name, profile_name)
    local profile = defaults.build_profiles[profile_name]
    local build_tree = resolve_profile_build_tree(profile_name)

    return {
        phase = "compile",
        scope = profile.scope,
        input = profile.compile_inputs,
        output = resolve_outputs(profile, build_tree),
        description = profile.compile_description,
        config = step_config.compile_defaults(profile_name),
        run = function(ctx)
            return runner.compile(ctx, step_name)
        end,
    }
end

local function link_step_def(step_name, profile_name)
    local profile = defaults.build_profiles[profile_name]
    local build_tree = resolve_profile_build_tree(profile_name)
    local link_outputs = profile.link_outputs
    if type(link_outputs) == "function" then
        link_outputs = link_outputs(build_tree)
    end

    return {
        phase = "bundle",
        scope = profile.scope,
        input = {
            build_tree .. "/.beez-clang-index.lua",
            build_tree .. "/.beez-clang-scripts/**",
        },
        output = link_outputs,
        description = profile.link_description,
        config = step_config.link_defaults(profile_name),
        run = function(ctx)
            return runner.link(ctx, step_name)
        end,
    }
end

local plugin_steps = {}

for step_name, profile_name in pairs(defaults.compile_step_profiles) do
    plugin_steps[step_name] = compile_step_def(step_name, profile_name)
end

for step_name, profile_name in pairs(defaults.link_step_profiles) do
    plugin_steps[step_name] = link_step_def(step_name, profile_name)
end

-- Beez clang-build plugin
--
-- Direct Clang compile + link using compile_commands.json from CMake configure.
-- Configure stays in coditary/conan (conan install + cmake --preset).
--
-- Steps (phase build, per profile scope):
--   compile:code / link:code       — Release/Debug app + all test binaries
--   compile:debug / link:debug     — Debug app only
--   compile:coverage / link:coverage
--   compile:sanitize / link:sanitize
--   compile:tsan / link:tsan
--   compile:fuzzer / link:fuzzer   — fuzz_lua_dsl
--
-- Env: BUILD_TYPE, CC, CXX, REPORTS_DIR
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/clang-build",
--             path = "./plugins/coditary/clang-build",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("clang-build", {
    version = "1.0.0",
    description = "Clang compile and link steps from compile_commands.json",
    organization = "coditary",
    steps = plugin_steps,
})
