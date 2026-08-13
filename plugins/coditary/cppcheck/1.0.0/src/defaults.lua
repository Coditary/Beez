local M = {}

M.patterns_analyze = {
    "src/**/*.cpp",
}

M.patterns_security = {
    "src/**/*.cpp",
    "src/**/*.hpp",
    "src/**/*.h",
    "include/**/*.cpp",
    "include/**/*.hpp",
    "include/**/*.h",
}

M.binary = "cppcheck"
M.std = "c++20"
M.enable = "warning,style,performance,portability"
M.include_paths = { "include" }
M.suppressions = {
    "missingIncludeSystem",
    "missingInclude",
    "unusedFunction",
}
M.inline_suppr = true
M.quiet = true
M.issue_path_pattern = "/(src|include|tests)/"
M.exclude_substrings = { "_deps" }
M.parallelism = 16
M.extra_args = {}
M.warnings_as_errors = false

M.log_prefix_analyze = "[cppcheck]"
M.log_prefix_security = "[cppcheck-security]"
M.log_prefix_check = "[cppcheck]"

M.worker_prefix_analyze = "cppcheck_"
M.worker_prefix_security = "cppsec_"
M.worker_prefix_check = "cppcheck_"

M.analyze_rev = "1"
M.security_rev = "1"
M.check_rev = "1"

M.profiles = {
    analyze = {
        patterns = M.patterns_analyze,
        log_prefix = M.log_prefix_analyze,
        worker_prefix = M.worker_prefix_analyze,
    },
    security = {
        patterns = M.patterns_security,
        log_prefix = M.log_prefix_security,
        worker_prefix = M.worker_prefix_security,
    },
}

return M
