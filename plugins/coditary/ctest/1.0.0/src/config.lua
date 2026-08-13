local defaults = require("src.defaults")
local env = require("src.env")

local M = {}

local function copy_string_array(values, fallback)
    if values == nil then
        return fallback
    end

    if type(values) ~= "table" then
        error("ctest array config fields must be tables of strings")
    end

    local copied = {}
    for _, entry in ipairs(values) do
        if type(entry) ~= "string" then
            error("ctest array config entries must be strings")
        end
        copied[#copied + 1] = entry
    end

    return copied
end

function M.resolve_build_tree(suite, step_cfg)
    if step_cfg.build_tree ~= nil then
        return step_cfg.build_tree
    end

    if suite.build_tree ~= nil then
        return suite.build_tree
    end

    if suite.build_tree_from_build_type then
        local build_type = step_cfg.build_type or env.env_or("BUILD_TYPE", "Release")
        return "build/build/" .. build_type
    end

    return defaults.debug_build_tree
end

function M.resolve(step_cfg, suite_name)
    local cfg = step_cfg or {}
    local suite = defaults.suites[suite_name]
    if suite == nil then
        error("unknown ctest suite: " .. tostring(suite_name))
    end

    local reports_dir = cfg.reports_dir or env.env_or("REPORTS_DIR", defaults.reports_dir)
    local build_tree = M.resolve_build_tree(suite, cfg)

    return {
        suite_name = suite_name,
        scope = suite.scope,
        mode = suite.mode,
        ctest_args = cfg.ctest_args or suite.ctest_args,
        build_tree = build_tree,
        binary_path = build_tree .. "/" .. suite.binary_rel,
        reports_dir = reports_dir,
        report_marker = cfg.report_marker or suite.report_marker,
        report_subdir = suite.report_subdir,
        report_txt = suite.report_txt,
        report_ok = suite.report_ok,
        script = cfg.script or suite.script,
        log_prefix = cfg.log_prefix or suite.log_prefix,
        test_rev = cfg.test_rev or defaults.test_rev,
        common_inputs = copy_string_array(cfg.common_inputs, defaults.common_inputs),
        extra_inputs = copy_string_array(cfg.extra_inputs, suite.extra_inputs or {}),
        extra_input_files = copy_string_array(cfg.extra_input_files, suite.extra_input_files or {}),
        ctest_timeout = cfg.ctest_timeout or suite.ctest_timeout,
    }
end

return M
