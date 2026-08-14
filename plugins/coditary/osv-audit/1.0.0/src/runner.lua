local config = require("src.config")
local command = require("src.command")

local M = {}

local function resolve_step_config(ctx, fallback)
    local from_context = ctx.get_config()
    if from_context ~= nil then
        return from_context
    end

    return fallback()
end

function M.audit_check(ctx)
    local step_cfg = resolve_step_config(ctx, require("src.step_config").audit_defaults)
    local cfg = config.resolve(step_cfg)
    local root = ctx.project_root

    local scanner_env = config.resolve_scanner_cmd(cfg, root)
    local mkdir_code = beez.shell.run(ctx, cfg.log_prefix, command.mkdir_security(cfg, root))
    if mkdir_code ~= 0 then
        return mkdir_code
    end

    print(cfg.log_prefix .. " vulnerability scan")
    print(cfg.log_prefix .. " SBOM: " .. cfg.sbom_json)
    print(cfg.log_prefix .. " lockfile: " .. cfg.lockfile)

    local scan_cmd = command.scan(cfg, root, scanner_env)
    local code, output = beez.shell.run(ctx, cfg.log_prefix, scan_cmd, { return_output = true })

    if code == 2 then
        print("error: osv-scanner not found in PATH")
        print("Install it with: " .. root .. "/" .. cfg.install_script)
        print("Or set auto_install = true in configure_step(\"osv_audit_check\", ...)")
        return 2
    end

    if code == 0 then
        print(cfg.log_prefix .. " passed (no known vulnerabilities in OSV)")
        return 0
    end

    if code == 1 then
        print(cfg.log_prefix .. " failed (known vulnerabilities found)")
        print("See " .. cfg.audit_report .. " for details.")
        return 1
    end

    print(cfg.log_prefix .. " failed (osv-scanner exit " .. tostring(code) .. ")")
    return code
end

return M
