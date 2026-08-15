local config = require("src.config")
local command = require("src.command")
local defaults = require("src.defaults")

local M = {}

local function build_command(cfg, root)
    if cfg.mode == "ctest" then
        return command.ctest(cfg, root)
    end

    if cfg.mode == "ctest_tee" then
        return command.ctest_tee(cfg, root)
    end

    if cfg.mode == "script" then
        return command.script(cfg, root)
    end

    error("unknown ctest mode: " .. tostring(cfg.mode))
end

function M.run(ctx, step_name)
    local suite_name = defaults.test_step_suites[step_name]
    if suite_name == nil then
        error("unknown test step: " .. tostring(step_name))
    end

    local suite = defaults.suites[suite_name]
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("ctest step config is missing")
    end

    local cfg = config.resolve(step_cfg, suite_name)
    local root = ctx.project_root

    print(cfg.log_prefix .. " " .. suite.description)
    return beez.shell.run(ctx, cfg.log_prefix, build_command(cfg, root))
end

return M
