local M = {}

M.patterns_all = {
    "src/**/*.cpp",
    "src/**/*.hpp",
    "src/**/*.h",
    "include/**/*.cpp",
    "include/**/*.hpp",
    "include/**/*.h",
    "tests/**/*.cpp",
    "tests/**/*.hpp",
    "tests/**/*.h",
}

M.patterns_src_cpp = {
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

M.binary = "clang-tidy"
M.header_filter = "(src|include|tests)/.*"
M.issue_path_pattern = "/(src|include|tests)/"
M.exclude_substrings = { "_deps" }
M.parallelism = 16
M.extra_args = {}
M.warnings_as_errors = false

M.checks_analyze =
    "-*,clang-analyzer-*,bugprone-*,cppcoreguidelines-*,performance-*"
M.checks_security = "-*,clang-analyzer-security-*,cert-*,misc-security-*"

M.log_prefix_lint = "[clang-tidy]"
M.log_prefix_analyze = "[analyze-tidy]"
M.log_prefix_security = "[security-tidy]"
M.log_prefix_check = "[clang-tidy]"

M.worker_prefix_lint = "tidy_"
M.worker_prefix_analyze = "analyze_"
M.worker_prefix_security = "sec_"
M.worker_prefix_check = "tidy_"

M.lint_rev = "3"
M.analyze_rev = "2"
M.security_rev = "2"
M.check_rev = "1"

M.profiles = {
    lint = {
        patterns = M.patterns_all,
        checks = nil,
        log_prefix = M.log_prefix_lint,
        worker_prefix = M.worker_prefix_lint,
    },
    analyze = {
        patterns = M.patterns_src_cpp,
        checks = M.checks_analyze,
        log_prefix = M.log_prefix_analyze,
        worker_prefix = M.worker_prefix_analyze,
    },
    security = {
        patterns = M.patterns_security,
        checks = M.checks_security,
        log_prefix = M.log_prefix_security,
        worker_prefix = M.worker_prefix_security,
    },
}

return M
