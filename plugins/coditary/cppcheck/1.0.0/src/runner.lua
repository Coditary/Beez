local config = require("src.config")
local cache = require("src.cache")
local step_config = require("src.step_config")

local M = {}

local function resolve_step_config(ctx, fallback)
    local from_context = ctx.get_config()
    if from_context ~= nil then
        return from_context
    end

    return fallback()
end

local function run_expanded(ctx, step_cfg)
    local runs = config.expand_runs(step_cfg)

    for _, run_cfg in ipairs(runs) do
        local code = cache.run(ctx, run_cfg)
        if code ~= 0 then
            return code
        end
    end

    return 0
end

function M.check(ctx)
    return run_expanded(ctx, resolve_step_config(ctx, step_config.check_defaults))
end

function M.analyze_check(ctx)
    return run_expanded(ctx, resolve_step_config(ctx, step_config.analyze_defaults))
end

function M.security_check(ctx)
    return run_expanded(ctx, resolve_step_config(ctx, step_config.security_defaults))
end

return M
