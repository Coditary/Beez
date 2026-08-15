local defaults = require("src.defaults")

local M = {}

local function normalize_config(config)
    if config == nil then
        return {}
    end

    if type(config) ~= "table" then
        error("osv-audit config must be a table")
    end

    return beez.data.clone(config)
end

local function read_string_field(config, field)
    if config == nil then
        return nil
    end

    local direct = config[field]
    if type(direct) == "string" and direct ~= "" then
        return direct
    end

    for key, value in pairs(config) do
        if tostring(key) == field and type(value) == "string" and value ~= "" then
            return value
        end
    end

    return nil
end

function M.resolve_scanner_path(config)
    local scanner = read_string_field(config, "osv_scanner")
    if scanner ~= nil then
        return scanner
    end

    local home = beez.env("HOME")
    if home ~= nil and home ~= "" then
        local local_scanner = home .. "/.local/bin/osv-scanner"
        if beez.fs.exists(local_scanner) then
            return local_scanner
        end
    end

    return nil
end

function M.resolve(config)
    local cfg = normalize_config(config)

    return {
        reports_dir = cfg.reports_dir or defaults.reports_dir,
        security_dir = cfg.security_dir or defaults.security_dir,
        audit_report = cfg.audit_report or defaults.audit_report,
        lockfile = cfg.lockfile or defaults.lockfile,
        sbom_json = cfg.sbom_json or defaults.sbom_json,
        install_script = cfg.install_script or defaults.install_script,
        osv_scanner = M.resolve_scanner_path(config),
        auto_install = cfg.auto_install == true,
        log_prefix = cfg.log_prefix or defaults.log_prefix,
        audit_rev = cfg.audit_rev or defaults.audit_rev,
    }
end

return M
