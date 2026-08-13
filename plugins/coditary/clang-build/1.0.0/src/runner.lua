local config = require("src.config")
local compdb = require("src.compdb")
local compile = require("src.compile")
local defaults = require("src.defaults")
local link = require("src.link")
local step_config = require("src.step_config")

local M = {}

local function resolve_step_config(ctx, fallback)
    local from_context = ctx.get_config()
    if from_context ~= nil then
        return from_context
    end

    return fallback()
end

function M.compile(ctx, step_name)
    local profile_name = defaults.compile_step_profiles[step_name]
    if profile_name == nil then
        error("unknown compile step: " .. tostring(step_name))
    end

    local fallback = function()
        return step_config.compile_defaults(profile_name)
    end

    local step_cfg = resolve_step_config(ctx, fallback)
    local cfg = config.resolve(step_cfg, profile_name, ctx.project_root)
    local profile = defaults.build_profiles[profile_name]

    print(cfg.log_prefix_compile .. " " .. profile.compile_description)

    local index, code = compdb.load_index(cfg, ctx.project_root, ctx)
    if index == nil then
        return code
    end

    return compile.run(ctx, cfg, index)
end

function M.link(ctx, step_name)
    local profile_name = defaults.link_step_profiles[step_name]
    if profile_name == nil then
        error("unknown link step: " .. tostring(step_name))
    end

    local fallback = function()
        return step_config.link_defaults(profile_name)
    end

    local step_cfg = resolve_step_config(ctx, fallback)
    local cfg = config.resolve(step_cfg, profile_name, ctx.project_root)
    local profile = defaults.build_profiles[profile_name]

    print(cfg.log_prefix_link .. " " .. profile.link_description)

    local index, code = compdb.load_index(cfg, ctx.project_root, ctx)
    if index == nil then
        return code
    end

    return link.run(ctx, cfg, profile, index, ctx.project_root)
end

return M
