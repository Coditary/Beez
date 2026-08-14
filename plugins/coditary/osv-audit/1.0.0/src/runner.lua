local config = require("src.config")
local command = require("src.command")

local M = {}

function M.audit_check(ctx)
    local step_cfg = ctx.get_config()
    if step_cfg == nil then
        error("osv-audit step config is missing")
    end

    local cfg = config.resolve(step_cfg)
    local root = ctx.project_root
    local scanner_path = cfg.osv_scanner

    if scanner_path == nil or scanner_path == "" then
        print("error: osv-scanner not found in PATH")
        print("Install it with: " .. root .. "/" .. cfg.install_script)
        print("Or set auto_install = true in configure_step(\"osv_audit_check\", ...)")
        return 2
    end

    local mkdir_code = beez.shell.run(ctx, cfg.log_prefix, command.mkdir_security(cfg, root))
    if mkdir_code ~= 0 then
        return mkdir_code
    end

    print(cfg.log_prefix .. " vulnerability scan")
    print(cfg.log_prefix .. " SBOM: " .. cfg.sbom_json)
    print(cfg.log_prefix .. " lockfile: " .. cfg.lockfile)

    local scan_cmd = command.scan(cfg, root, scanner_path)
    local code = beez.shell.run(ctx, cfg.log_prefix, scan_cmd)

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
