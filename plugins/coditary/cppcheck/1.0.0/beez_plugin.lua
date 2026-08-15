local defaults = require("src.defaults")

plugin("cppcheck", {
    version = "1.0.0",
    description = "Incremental cppcheck static analysis",
    organization = "coditary",

    config = {
        defaults = {
            binary = defaults.binary,
            std = defaults.std,
            enable = defaults.enable,
            include_paths = defaults.include_paths,
            suppressions = defaults.suppressions,
            inline_suppr = defaults.inline_suppr,
            quiet = defaults.quiet,
            issue_path_pattern = defaults.issue_path_pattern,
            exclude_substrings = defaults.exclude_substrings,
            parallelism = defaults.parallelism,
            extra_args = defaults.extra_args,
            warnings_as_errors = defaults.warnings_as_errors,
        },

        profile_defs = {
            analyze = {
                patterns = defaults.patterns_analyze,
                log_prefix = defaults.log_prefix_analyze,
                worker_prefix = defaults.worker_prefix_analyze,
                check_rev = defaults.analyze_rev,
            },
            security = {
                patterns = defaults.patterns_security,
                log_prefix = defaults.log_prefix_security,
                worker_prefix = defaults.worker_prefix_security,
                check_rev = defaults.security_rev,
            },
        },

        finalize = function(resolved)
            local config = require("src.config")
            resolved.enable = config.normalize_enable(resolved.enable)
            return resolved
        end,
    },

    steps = {
        cppcheck_check = {
            phase = "quality",
            scope = "analyze",
            input = defaults.patterns_security,
            description = "cppcheck (configurable profiles)",
            config = {
                profile = "analyze",
                profiles = { "analyze" },
            },
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        cppcheck_analyze_check = {
            phase = "quality",
            scope = "analyze",
            input = defaults.patterns_analyze,
            description = "cppcheck on src/ (incremental)",
            config = { profile = "analyze" },
            run = function(ctx)
                return require("src.runner").analyze_check(ctx)
            end,
        },

        cppcheck_security_check = {
            phase = "verify",
            scope = "security",
            input = defaults.patterns_security,
            description = "cppcheck security scan (incremental)",
            config = { profile = "security" },
            run = function(ctx)
                return require("src.runner").security_check(ctx)
            end,
        },
    },
})
