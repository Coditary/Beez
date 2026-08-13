local defaults = require("src.defaults")
local step_config = require("src.step_config")

-- Beez clang-tidy plugin
--
-- Steps (after load):
--   check           — configurable clang-tidy (profiles or custom checks array)
--   lint_check      — preset profile: lint (.clang-tidy defaults)
--   analyze_check   — preset profile: analyzer / bugprone / guidelines
--   security_check  — preset profile: security checks
--
-- Required configure_step override (compile db path):
--   configure_step("check", {
--       compdb = "build/build/Release",
--       profiles = { "lint", "analyze", "security" },
--       check_rev = "1",
--   })
--
-- Custom checks (array joins to --checks=-*,name1,name2):
--   configure_step("check", {
--       compdb = "build/build/Release",
--       checks = { "bugprone-*", "modernize-use-nullptr" },
--       patterns = { "src/**/*.cpp" },
--   })
--
-- Optional fields: patterns, header_filter, binary, extra_args,
--   issue_path_pattern, exclude_substrings, log_prefix, worker_prefix
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/clang-tidy",
--             path = "./plugins/coditary/clang-tidy",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("clang-tidy", {
    version = "1.0.0",
    description = "Incremental clang-tidy lint, analyzer, and security checks",
    organization = "coditary",

    steps = {
        check = {
            phase = "qa",
            scope = "code",
            input = defaults.patterns_all,
            description = "clang-tidy check (configurable profiles or checks)",
            config = step_config.check_defaults(),
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        lint_check = {
            phase = "qa",
            scope = "lint",
            input = defaults.patterns_all,
            description = "clang-tidy lint (incremental)",
            config = step_config.lint_defaults(),
            run = function(ctx)
                return require("src.runner").lint_check(ctx)
            end,
        },

        analyze_check = {
            phase = "qa",
            scope = "analyze",
            input = defaults.patterns_src_cpp,
            description = "clang-tidy analyzer checks (incremental)",
            config = step_config.analyze_defaults(),
            run = function(ctx)
                return require("src.runner").analyze_check(ctx)
            end,
        },

        security_check = {
            phase = "qa",
            scope = "security",
            input = defaults.patterns_security,
            description = "clang-tidy security checks (incremental)",
            config = step_config.security_defaults(),
            run = function(ctx)
                return require("src.runner").security_check(ctx)
            end,
        },
    },
})
