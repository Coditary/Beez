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
        config = {
            profile = profile_name,
            cache_key = profile_name .. ":compile",
        },
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
        config = {
            profile = profile_name,
            cache_key = profile_name .. ":link",
        },
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

plugin("clang", {
    version = "1.0.0",
    description = "Clang compile and link steps from compile_commands.json",
    organization = "coditary",

    config = {
        defaults = {
            index_script = defaults.index_script,
            python_binary = defaults.python_binary,
            compile_rev = defaults.compile_rev,
            link_rev = defaults.link_rev,
            log_prefix_compile = defaults.log_prefix_compile,
            log_prefix_link = defaults.log_prefix_link,
            parallelism = 16,
        },
    },

    steps = plugin_steps,
})
