local defaults = require("src.defaults")

local runner = require("src.runner")

local function resolve_build_tree(suite_name)
    local suite = defaults.suites[suite_name]
    if suite.build_tree ~= nil then
        return suite.build_tree
    end

    if suite.build_tree_from_build_type then
        local build_type = beez.env_or("BUILD_TYPE", "Release")
        return "build/build/" .. build_type
    end

    return defaults.debug_build_tree
end

local function build_inputs(suite_name)
    local suite = defaults.suites[suite_name]
    local build_tree = resolve_build_tree(suite_name)
    local inputs = {
        build_tree .. "/" .. suite.binary_rel,
    }

    for _, pattern in ipairs(defaults.common_inputs) do
        inputs[#inputs + 1] = pattern
    end

    for _, pattern in ipairs(suite.extra_inputs or {}) do
        inputs[#inputs + 1] = pattern
    end

    for _, path in ipairs(suite.extra_input_files or {}) do
        inputs[#inputs + 1] = path
    end

    return inputs
end

local function build_outputs(suite_name)
    local suite = defaults.suites[suite_name]
    local reports_dir = beez.env_or("REPORTS_DIR", defaults.reports_dir)
    local build_tree = resolve_build_tree(suite_name)

    if suite.report_outputs ~= nil then
        return suite.report_outputs(reports_dir, build_tree)
    end

    if suite.mode == "ctest_tee" then
        return { reports_dir .. "/" .. suite.report_subdir .. "/" .. suite.report_ok }
    end

    return { suite.report_marker }
end

local plugin_steps = {}

for step_name, suite_name in pairs(defaults.test_step_suites) do
    local suite = defaults.suites[suite_name]

    plugin_steps[step_name] = {
        phase = "test",
        scope = suite.scope,
        input = build_inputs(suite_name),
        output = build_outputs(suite_name),
        description = suite.description,
        config = {
            log_prefix = suite.log_prefix,
        },
        run = function(ctx)
            return runner.run(ctx, step_name)
        end,
    }
end

plugin("ctest", {
    version = "1.0.0",
    description = "CTest suite runs for unit, integration, coverage, and sanitizer workflows",
    organization = "coditary",

    config = {
        defaults = {
            test_rev = defaults.test_rev,
            common_inputs = defaults.common_inputs,
        },
    },

    steps = plugin_steps,
})
