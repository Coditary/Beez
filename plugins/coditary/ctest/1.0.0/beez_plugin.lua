local defaults = require("src.defaults")
local env = require("src.env")
local step_config = require("src.step_config")

local runner = require("src.runner")

local function resolve_build_tree(suite_name)
    local suite = defaults.suites[suite_name]
    if suite.build_tree ~= nil then
        return suite.build_tree
    end

    if suite.build_tree_from_build_type then
        local build_type = env.env_or("BUILD_TYPE", "Release")
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
    local reports_dir = env.env_or("REPORTS_DIR", defaults.reports_dir)
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
        config = step_config.suite_defaults(suite_name),
        run = function(ctx)
            return runner.run(ctx, step_name)
        end,
    }
end

-- Beez ctest plugin
--
-- Steps:
--   test:unit         — ctest -L unit
--   test:integration  — ctest -L integration
--   test:system       — ctest -L system
--   test:performance  — ctest -L performance
--   test:coverage     — coverage test script + gcda outputs
--   test:sanitize     — full ctest under ASan/UBSan with report
--   test:tsan         — full ctest under TSan with report
--
-- Env: BUILD_TYPE (code-scope build tree), REPORTS_DIR
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/ctest",
--             path = "./plugins/coditary/ctest",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("ctest", {
    version = "1.0.0",
    description = "CTest suite runs for unit, integration, coverage, and sanitizer workflows",
    organization = "coditary",
    steps = plugin_steps,
})
