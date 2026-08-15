local defaults = require("src.defaults")

plugin("osv-audit", {
    version = "1.0.0",
    description = "OSV vulnerability audit for Conan lockfiles",
    organization = "coditary",

    config = {
        defaults = {
            reports_dir = defaults.reports_dir,
            security_dir = defaults.security_dir,
            audit_report = defaults.audit_report,
            lockfile = defaults.lockfile,
            sbom_json = defaults.sbom_json,
            install_script = defaults.install_script,
            log_prefix = defaults.log_prefix,
            audit_rev = defaults.audit_rev,
        },
    },

    steps = {
        osv_audit_check = {
            phase = "verify",
            scope = "audit",
            input = {
                defaults.lockfile,
                defaults.sbom_json,
            },
            output = {
                defaults.audit_report,
            },
            description = "Dependency vulnerability scan (OSV)",
            config = {},
            run = function(ctx)
                return require("src.runner").audit_check(ctx)
            end,
        },
    },
})
