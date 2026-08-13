local defaults = require("src.defaults")
local step_config = require("src.step_config")

-- Beez OSV audit plugin
--
-- Steps:
--   osv_audit_check — scan Conan lockfile with osv-scanner
--
-- configure_step("osv_audit_check", {
--     lockfile = "conan.lock",
--     sbom_json = "report/sbom/cyclonedx.json",
--     audit_rev = "1",
-- })
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/osv-audit",
--             path = "./plugins/coditary/osv-audit",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("osv-audit", {
    version = "1.0.0",
    description = "OSV vulnerability audit for Conan lockfiles",
    organization = "coditary",

    steps = {
        osv_audit_check = {
            phase = "qa",
            scope = "supply",
            input = {
                defaults.lockfile,
                defaults.sbom_json,
            },
            output = {
                defaults.audit_report,
            },
            description = "Dependency vulnerability scan (OSV)",
            config = step_config.audit_defaults(),
            run = function(ctx)
                return require("src.runner").audit_check(ctx)
            end,
        },
    },
})
