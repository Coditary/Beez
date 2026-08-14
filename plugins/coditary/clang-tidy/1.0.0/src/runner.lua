local config = require("src.config")
local cache = require("src.cache")

local M = {}

local function run_expanded(ctx, step_cfg)
    for _, run_cfg in ipairs(config.expand_runs(step_cfg)) do
        local code = cache.run(ctx, run_cfg)
        if code ~= 0 then
            return code
        end
    end

    return 0
end

function M.check(ctx)
    return run_expanded(ctx, ctx.get_config())
end

function M.lint_check(ctx)
    return run_expanded(ctx, ctx.get_config())
end

function M.analyze_check(ctx)
    return run_expanded(ctx, ctx.get_config())
end

function M.security_check(ctx)
    return run_expanded(ctx, ctx.get_config())
end

return M
