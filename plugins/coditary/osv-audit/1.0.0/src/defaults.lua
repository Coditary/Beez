local M = {}

M.reports_dir = "report"
M.security_dir = "report/security"
M.audit_report = "report/security/dependency-audit.txt"
M.lockfile = "conan.lock"
M.sbom_json = "report/sbom/cyclonedx.json"

M.install_script = "scripts/ci-install-osv-scanner.sh"

M.log_prefix = "[osv-audit]"

M.audit_rev = "1"

return M
