local config = require("src.config")
local cache = require("src.cache")
local defaults = require("src.defaults")
local step_config = require("src.step_config")

local M = {}

local function resolve_step_config(ctx)
    local from_context = ctx.get_config()
    if from_context ~= nil then
        return from_context
    end

    return step_config.defaults()
end

function M.check(ctx)
    return cache.run(ctx, config.resolve_for_mode(resolve_step_config(ctx), "check"), "check")
end

function M.apply(ctx)
    return cache.run(ctx, config.resolve_for_mode(resolve_step_config(ctx), "apply"), "apply")
end

return M
