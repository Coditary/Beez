
local M = {}

function M.mkdir_security(config, root)
    return "mkdir -p " .. beez.char.quote(root .. "/" .. config.security_dir)
end

function M.scan(config, root, scanner_env)
    local lock_path = root .. "/" .. config.lockfile
    local report_path = root .. "/" .. config.audit_report

    local parts = {
        scanner_env,
        "&&",
        "\"$OSV_SCANNER\" scan",
        "--lockfile=" .. beez.char.quote(lock_path),
        "--format=vertical",
        "--verbosity=warn",
        "2>&1 | tee " .. beez.char.quote(report_path),
    }

    return table.concat(parts, " ")
end

return M
