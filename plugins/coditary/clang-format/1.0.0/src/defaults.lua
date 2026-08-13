local M = {}

M.patterns = {
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

M.binary = "clang-format"
M.format_rev = "1"
M.log_prefix_check = "[clang-format]"
M.log_prefix_apply = "[clang-format]"
M.worker_prefix_check = "clang_fmt_"
M.worker_prefix_apply = "clang_apply_"
M.werror = true

return M
