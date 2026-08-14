local defaults = require("src.defaults")

plugin("clang-tidy", {
    version = "1.0.0",
    description = "Incremental clang-tidy lint, analyzer, and security checks",
    organization = "coditary",

    config = {
        defaults = {
            binary = defaults.binary,
            header_filter = defaults.header_filter,
            issue_path_pattern = defaults.issue_path_pattern,
            exclude_substrings = defaults.exclude_substrings,
            extra_args = defaults.extra_args,
            parallelism = defaults.parallelism,
            warnings_as_errors = defaults.warnings_as_errors,
        },

        profile_defs = {
            lint = {
                patterns = defaults.patterns_all,
                log_prefix = defaults.log_prefix_lint,
                worker_prefix = defaults.worker_prefix_lint,
                lint_rev = defaults.lint_rev,
            },
            analyze = {
                patterns = defaults.patterns_src_cpp,
                checks = defaults.checks_analyze,
                log_prefix = defaults.log_prefix_analyze,
                worker_prefix = defaults.worker_prefix_analyze,
                analyze_rev = defaults.analyze_rev,
            },
            security = {
                patterns = defaults.patterns_security,
                checks = defaults.checks_security,
                log_prefix = defaults.log_prefix_security,
                worker_prefix = defaults.worker_prefix_security,
                security_rev = defaults.security_rev,
            },
        },

        finalize = function(resolved)
            local config = require("src.config")
            if resolved.compdb == nil or resolved.compdb == "" then
                resolved.compdb = config.default_compdb()
            end

            local extra_args = {}
            if resolved.extra_args ~= nil then
                for _, argument in ipairs(resolved.extra_args) do
                    extra_args[#extra_args + 1] = argument
                end
            end

            if resolved.warnings_as_errors then
                extra_args[#extra_args + 1] = "--warnings-as-errors=*"
            end

            resolved.extra_args = extra_args
            resolved.checks = config.normalize_checks(resolved.checks)
            return resolved
        end,
    },

    steps = {
        check = {
            phase = "quality",
            scope = "lint",
            input = defaults.patterns_all,
            description = "clang-tidy check (configurable profiles or checks)",
            config = {
                profile = "lint",
                profiles = { "lint" },
                check_rev = defaults.check_rev,
                log_prefix = defaults.log_prefix_check,
                worker_prefix = defaults.worker_prefix_check,
            },
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        lint_check = {
            phase = "quality",
            scope = "lint",
            input = defaults.patterns_all,
            description = "clang-tidy lint (incremental)",
            config = {
                profile = "lint",
                profiles = { "lint" },
            },
            run = function(ctx)
                return require("src.runner").lint_check(ctx)
            end,
        },

        analyze_check = {
            phase = "quality",
            scope = "analyze",
            input = defaults.patterns_src_cpp,
            description = "clang-tidy analyzer checks (incremental)",
            config = {
                profile = "analyze",
                profiles = { "analyze" },
            },
            run = function(ctx)
                return require("src.runner").analyze_check(ctx)
            end,
        },

        security_check = {
            phase = "verify",
            scope = "security",
            input = defaults.patterns_security,
            description = "clang-tidy security checks (incremental)",
            config = {
                profile = "security",
                profiles = { "security" },
            },
            run = function(ctx)
                return require("src.runner").security_check(ctx)
            end,
        },
    },
})
