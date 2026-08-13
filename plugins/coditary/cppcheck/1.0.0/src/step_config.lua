local defaults = require("src.defaults")

local M = {}

function M.check_defaults()
    return {
        profiles = { "analyze" },
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
        log_prefix = defaults.log_prefix_check,
        worker_prefix = defaults.worker_prefix_check,
        check_rev = defaults.check_rev,
    }
end

function M.analyze_defaults()
    return {
        profiles = { "analyze" },
        analyze_rev = defaults.analyze_rev,
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
        log_prefix = defaults.log_prefix_analyze,
        worker_prefix = defaults.worker_prefix_analyze,
    }
end

function M.security_defaults()
    return {
        profiles = { "security" },
        security_rev = defaults.security_rev,
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
        log_prefix = defaults.log_prefix_security,
        worker_prefix = defaults.worker_prefix_security,
    }
end

return M
