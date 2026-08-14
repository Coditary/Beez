local config = require("src.config")
local cache = require("src.cache")

local M = {}

function M.check(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("cppcheck step config is missing")
    end

    for _, run_cfg in ipairs(config.expand_runs(step_cfg)) do
        local code = cache.run(ctx, run_cfg)
        if code ~= 0 then
            return code
        end
    end

    return 0
end

function M.analyze_check(ctx)
    return cache.run(ctx, ctx.get_config())
end

function M.security_check(ctx)
    return cache.run(ctx, ctx.get_config())
end

return M
