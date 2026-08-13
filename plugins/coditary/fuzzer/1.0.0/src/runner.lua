local config = require("src.config")
local command = require("src.command")
local defaults = require("src.defaults")
local shell = require("src.shell")

local M = {}

local function resolve_step_config(ctx, run_name)
    local from_context = ctx.get_config()
    if from_context ~= nil then
        return from_context
    end

    return require("src.step_config").run_defaults(run_name)
end

local function build_command(cfg, root)
    if cfg.mode == "direct" then
        return command.direct(cfg, root)
    end

    return command.script(cfg, root)
end

function M.run(ctx, step_name)
    local run_name = defaults.fuzz_step_runs[step_name]
    if run_name == nil then
        error("unknown fuzz step: " .. tostring(step_name))
    end

    local run_def = defaults.runs[run_name]
    local step_cfg = resolve_step_config(ctx, run_name)
    local cfg = config.resolve(step_cfg, run_name)

    print(cfg.log_prefix .. " " .. run_def.description)
    return shell.run(ctx, cfg.log_prefix, build_command(cfg, ctx.project_root))
end

return M
