local defaults = require("src.defaults")

local M = {}

function M.audit_defaults()
    return {
        audit_rev = defaults.audit_rev,
        lockfile = defaults.lockfile,
        sbom_json = defaults.sbom_json,
        audit_report = defaults.audit_report,
        security_dir = defaults.security_dir,
        log_prefix = defaults.log_prefix,
    }
end

return M
