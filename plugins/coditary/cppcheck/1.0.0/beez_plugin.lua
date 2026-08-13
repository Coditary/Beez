local defaults = require("src.defaults")
local step_config = require("src.step_config")

-- Beez cppcheck plugin
--
-- Steps (after load):
--   cppcheck_check           — configurable profiles (analyze, security)
--   cppcheck_analyze_check   — src/**/*.cpp
--   cppcheck_security_check  — src/ + include/ security scan
--
-- configure_step("cppcheck_check", {
--     profiles = { "analyze", "security" },
--     check_rev = "1",
-- })
--
-- configure_step("check", {
--     enable = { "warning", "style", "performance", "portability" },
--     include_paths = { "include" },
--     patterns = { "src/**/*.cpp" },
-- })
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/cppcheck",
--             path = "./plugins/coditary/cppcheck",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("cppcheck", {
    version = "1.0.0",
    description = "Incremental cppcheck static analysis",
    organization = "coditary",

    steps = {
        cppcheck_check = {
            phase = "qa",
            scope = "code",
            input = defaults.patterns_security,
            description = "cppcheck (configurable profiles)",
            config = step_config.check_defaults(),
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        cppcheck_analyze_check = {
            phase = "qa",
            scope = "cppcheck_analyze",
            input = defaults.patterns_analyze,
            description = "cppcheck on src/ (incremental)",
            config = step_config.analyze_defaults(),
            run = function(ctx)
                return require("src.runner").analyze_check(ctx)
            end,
        },

        cppcheck_security_check = {
            phase = "qa",
            scope = "cppcheck_security",
            input = defaults.patterns_security,
            description = "cppcheck security scan (incremental)",
            config = step_config.security_defaults(),
            run = function(ctx)
                return require("src.runner").security_check(ctx)
            end,
        },
    },
})
