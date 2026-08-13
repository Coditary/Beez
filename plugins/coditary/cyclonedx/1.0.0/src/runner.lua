local config = require("src.config")
local command = require("src.command")
local shell = require("src.shell")

local M = {}

local function resolve_step_config(ctx, fallback)
    local from_context = ctx.get_config()
    if from_context ~= nil then
        return from_context
    end

    return fallback()
end

function M.check(ctx)
    local step_cfg = resolve_step_config(ctx, require("src.step_config").check_defaults)
    local cfg = config.resolve(step_cfg)

    print(cfg.log_prefix_check .. " validating " .. cfg.cyclonedx_json)
    return shell.run(ctx, cfg.log_prefix_check, command.check(cfg, ctx.project_root))
end

function M.merge(ctx)
    local step_cfg = resolve_step_config(ctx, require("src.step_config").merge_defaults)
    local cfg = config.resolve(step_cfg)
    local root = ctx.project_root

    print(cfg.log_prefix_merge .. " merging BOM inputs:")
    for _, input in ipairs(cfg.merge_inputs) do
        print("  - " .. input)
    end

    local code = shell.run(ctx, cfg.log_prefix_merge, command.mkdir_sbom_dir(cfg, root))
    if code ~= 0 then
        return code
    end

    local merge_cmd
    if cfg.use_cli_merge then
        merge_cmd = command.merge_cli(cfg, root)
    else
        merge_cmd = command.merge_python(cfg, root)
    end

    code = shell.run(ctx, cfg.log_prefix_merge, merge_cmd)
    if code == 0 then
        print(cfg.log_prefix_merge .. " written to " .. cfg.merged_json)
    end

    return code
end

return M
