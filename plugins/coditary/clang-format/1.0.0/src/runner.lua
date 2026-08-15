local config = require("src.config")
local cache = require("src.cache")

local M = {}

function M.check(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("clang-format step config is missing")
    end

    return cache.run(ctx, config.resolve_for_mode(step_cfg, "check"), "check")
end

function M.apply(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("clang-format step config is missing")
    end

    return cache.run(ctx, config.resolve_for_mode(step_cfg, "apply"), "apply")
end

return M
