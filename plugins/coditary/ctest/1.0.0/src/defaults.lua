local M = {}

M.reports_dir = "report"
M.debug_build_tree = "build/build/Debug"
M.coverage_build_tree = "build/build/Coverage"
M.sanitize_build_tree = "build/build/Sanitize"
M.tsan_build_tree = "build/build/Tsan"
M.coverage_stamp = M.coverage_build_tree .. "/.beez-coverage-configured"

M.common_inputs = {
    "src/**/*.cpp",
    "include/**/*.hpp",
}

M.test_rev = "4"

M.suites = {
    unit = {
        scope = "test",
        mode = "ctest",
        ctest_args = "-L unit",
        binary_rel = "tests/unit/beez_tests",
        extra_inputs = { "tests/unit/**/*.cpp" },
        report_marker = "report/test/unit.ok",
        build_tree_from_build_type = true,
        log_prefix = "[ctest-unit]",
        description = "Run unit tests via ctest",
    },

    integration = {
        scope = "test",
        mode = "ctest",
        ctest_args = "-L integration",
        binary_rel = "tests/integration/beez_integration_tests",
        extra_inputs = { "tests/integration/**/*.cpp" },
        report_marker = "report/test/integration.ok",
        build_tree_from_build_type = true,
        log_prefix = "[ctest-integration]",
        description = "Run integration tests via ctest",
    },

    system = {
        scope = "test",
        mode = "ctest",
        ctest_args = "-L system -E FuzzCorpusSeedsDoNotCrashBeez",
        binary_rel = "tests/system/beez_system_tests",
        extra_inputs = { "tests/system/**/*.cpp" },
        report_marker = "report/test/system.ok",
        build_tree_from_build_type = true,
        log_prefix = "[ctest-system]",
        description = "Run system tests via ctest (excludes slow fuzz-corpus robustness)",
    },

    performance = {
        scope = "test",
        mode = "ctest",
        ctest_args = "-L performance",
        binary_rel = "tests/performance/beez_perf_tests",
        extra_inputs = { "tests/performance/**/*.cpp" },
        report_marker = "report/test/performance.ok",
        build_tree_from_build_type = true,
        log_prefix = "[ctest-performance]",
        description = "Run performance tests via ctest",
    },

    coverage = {
        scope = "coverage",
        mode = "script",
        script = "scripts/coverage-test.sh",
        binary_rel = "tests/unit/beez_tests",
        extra_inputs = { "tests/**/*.cpp" },
        extra_input_files = { M.coverage_stamp },
        build_tree = M.coverage_build_tree,
        log_prefix = "[ctest-coverage]",
        description = "Run unit tests and capture coverage data (coverage-test.sh)",
        report_outputs = function(reports_dir, build_tree)
            return {
                reports_dir .. "/test/coverage-test-report.ok",
                reports_dir .. "/test/coverage-test-report.txt",
                build_tree .. "/**/*.gcda",
            }
        end,
    },

    sanitize = {
        scope = "sanitize",
        mode = "ctest_tee",
        binary_rel = "tests/unit/beez_tests",
        extra_inputs = { "tests/**/*.cpp" },
        build_tree = M.sanitize_build_tree,
        report_subdir = "sanitize",
        report_txt = "sanitize-report.txt",
        report_ok = "sanitize-report.ok",
        log_prefix = "[ctest-sanitize]",
        description = "Run tests under ASan/UBSan",
    },

    tsan = {
        scope = "tsan",
        mode = "ctest_tee",
        binary_rel = "tests/unit/beez_tests",
        extra_inputs = { "tests/**/*.cpp" },
        build_tree = M.tsan_build_tree,
        report_subdir = "tsan",
        report_txt = "tsan-report.txt",
        report_ok = "tsan-report.ok",
        log_prefix = "[ctest-tsan]",
        description = "Run tests under ThreadSanitizer",
    },

    robustness = {
        scope = "test",
        mode = "ctest",
        ctest_args = "-R 'SystemRobustnessTest|SystemNegativeFixtureTest|SystemCacheAdversarialTest|SystemDslFieldMatrixTest'",
        binary_rel = "tests/system/beez_system_tests",
        extra_inputs = {
            "tests/system/**/*.cpp",
            "tests/fuzz/corpus/lua_dsl/*.lua",
        },
        report_marker = "report/test/robustness.ok",
        build_tree_from_build_type = true,
        log_prefix = "[ctest-robustness]",
        description = "Run robustness system tests (slow; fuzz corpus seeds)",
        ctest_timeout = 1800,
    },
}

M.test_step_suites = {
    ["test:unit"] = "unit",
    ["test:integration"] = "integration",
    ["test:system"] = "system",
    ["test:performance"] = "performance",
    ["test:coverage"] = "coverage",
    ["test:sanitize"] = "sanitize",
    ["test:tsan"] = "tsan",
    ["test:robustness"] = "robustness",
}

return M
