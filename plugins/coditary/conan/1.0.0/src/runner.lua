local config = require("src.config")
local command = require("src.command")
local defaults = require("src.defaults")

local M = {}

local function run_graph(ctx, cfg)
    print(cfg.log_prefix_graph .. " exporting Conan dependency graph")
    local code = beez.shell.run(ctx, cfg.log_prefix_graph, command.mkdir_sbom_dir(cfg, ctx.project_root))
    if code ~= 0 then
        return code
    end

    return beez.shell.run(ctx, cfg.log_prefix_graph, command.graph_info(cfg, ctx.project_root))
end

function M.graph_export(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("conan graph export step config is missing")
    end

    local cfg = config.resolve_supply(step_cfg, ctx.project_root)

    local code = run_graph(ctx, cfg)
    if code == 0 then
        print(cfg.log_prefix_graph .. " written to " .. cfg.graph_json)
    end

    return code
end

function M.lock_create(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("conan lock create step config is missing")
    end

    local cfg = config.resolve_supply(step_cfg, ctx.project_root)

    print(cfg.log_prefix_lock .. " creating Conan lockfile")
    local code = beez.shell.run(ctx, cfg.log_prefix_lock, command.lock_create(cfg, ctx.project_root))
    if code == 0 then
        print(cfg.log_prefix_lock .. " written to " .. cfg.lockfile)
    end

    return code
end

function M.sbom_export(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("conan sbom export step config is missing")
    end

    local cfg = config.resolve_supply(step_cfg, ctx.project_root)

    local code = run_graph(ctx, cfg)
    if code ~= 0 then
        return code
    end

    print(cfg.log_prefix_sbom .. " converting graph to CycloneDX")
    code = beez.shell.run(ctx, cfg.log_prefix_sbom, command.cyclonedx_convert(cfg, ctx.project_root))
    if code == 0 then
        print(cfg.log_prefix_sbom .. " written to " .. cfg.cyclonedx_json)
    end

    return code
end

function M.install(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("conan install step config is missing")
    end

    local profile_name = step_cfg.profile or "code"
    local cfg = config.resolve_build(step_cfg, ctx.project_root, profile_name)

    print(cfg.log_prefix_install .. " installing dependencies (" .. cfg.build_type .. ")")
    return beez.shell.run(ctx, cfg.log_prefix_install, command.install(cfg, ctx.project_root))
end

function M.configure(ctx, step_name)
    local profile_name = defaults.configure_step_profiles[step_name]
    if profile_name == nil then
        error("unknown configure step: " .. tostring(step_name))
    end

    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("conan configure step config is missing")
    end

    local cfg = config.resolve_build(step_cfg, ctx.project_root, profile_name)
    local profile = defaults.build_profiles[profile_name]

    local description = profile.configure_description
    if type(description) == "function" then
        description = description(cfg.build_type)
    end

    print(cfg.log_prefix_configure .. " " .. description)
    return beez.shell.run(ctx, cfg.log_prefix_configure, command.configure(cfg, ctx.project_root))
end

return M
