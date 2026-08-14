local defaults = require("src.defaults")

local M = {}

function M.resolve(config)
    local cfg = config or {}

    return {
        reports_dir = cfg.reports_dir or defaults.reports_dir,
        security_dir = cfg.security_dir or defaults.security_dir,
        audit_report = cfg.audit_report or defaults.audit_report,
        lockfile = cfg.lockfile or defaults.lockfile,
        sbom_json = cfg.sbom_json or defaults.sbom_json,
        install_script = cfg.install_script or defaults.install_script,
        osv_scanner = cfg.osv_scanner,
        auto_install = cfg.auto_install == true,
        log_prefix = cfg.log_prefix or defaults.log_prefix,
        audit_rev = cfg.audit_rev or defaults.audit_rev,
    }
end

function M.resolve_scanner_path_expr(config, root)
    if config.osv_scanner ~= nil and config.osv_scanner ~= "" then
        return beez.char.quote(config.osv_scanner)
    end

    local install = beez.char.quote(root .. "/" .. config.install_script)
    local auto = config.auto_install and "1" or "0"

    return "$(" ..
        "OSV_SCANNER_AUTO_INSTALL=" .. auto ..
        " bash -c '" ..
        "if [[ -n \"${OSV_SCANNER:-}\" && -x \"${OSV_SCANNER}\" ]]; then echo \"${OSV_SCANNER}\";" ..
        " elif [[ -x \"${HOME}/.local/bin/osv-scanner\" ]]; then echo \"${HOME}/.local/bin/osv-scanner\";" ..
        " elif command -v osv-scanner >/dev/null 2>&1; then echo osv-scanner;" ..
        " elif [[ \"${OSV_SCANNER_AUTO_INSTALL:-0}\" == \"1\" ]]; then bash " .. install ..
        " && echo \"${HOME}/.local/bin/osv-scanner\";" ..
        " else exit 2; fi'" .. ")"
end

function M.resolve_scanner_cmd(config, root)
    return "OSV_SCANNER=" .. M.resolve_scanner_path_expr(config, root)
end

return M
