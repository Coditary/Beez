-- Beez — real project pipeline (repo root):
--
--   beez build              Conan + CMake configure, compile, run all test suites
--   beez quality            format-check, clang-tidy lint, analyze, security
--   beez all                full QS pipeline (build + quality + coverage + sanitize + fuzz)
--   beez debug              Debug configure + build
--   beez coverage           Coverage configure, build, tests, HTML report
--   beez sanitize           ASan/UBSan configure, build, tests
--   beez tsan               ThreadSanitizer configure, build, tests
--   beez fuzzer_smoke       Build fuzzer + short fuzz run (FUZZER_TIME, default 30s)
--   beez fuzzer_corpus      Build fuzzer + longer corpus run (60s)
--   beez clean              remove build/, report/, .cache/
--   beez format             apply clang-format + cmake-format (incremental)
--   beez clang_tidy         clang-tidy only (same as qa:lint step)
--   beez clean_cache        clear .cache/ only
--
-- Incremental QA uses per-file success cache (.cache/success/).
-- Build/test steps use step cache (.cache/) keyed on inputs/outputs.
-- Bump lint_rev / analyze_rev / security_rev in configure_step after toolchain changes.
-- BUILD_TYPE / CONAN_PROFILE: process env or .env (default Release / clang-release).
--
-- Scopes group steps for workflow/CLI selection (phase + scope), not file domains.
--   code     — programming: configure (Release), build, test, qa, format:apply
--   debug    — Debug configure + build (isolated from Release configure)
--   coverage / sanitize / tsan / fuzz — specialized code toolchains (own configure tree)
--   smoke / corpus         — fuzz run variants (phase fuzz, separate workflow targets)
--   repo                   — repository-wide clean (future: book, docs, …)
-- Steps in the same phase+scope are ordered via order(); independent steps run in parallel.

beez.config(require("config"))

local function env_or(key, default)
    local value = beez.env(key)
    if value == nil then
        return default
    end
    return value
end

local BUILD_TYPE = env_or("BUILD_TYPE", "Release")
local CONAN_PROFILE = env_or("CONAN_PROFILE", "clang-release")
local BUILD_TREE = "build/build/" .. BUILD_TYPE
local CMAKE_PRESET = (BUILD_TYPE == "Debug") and "conan-debug" or "conan-release"
local DEBUG_BUILD_TREE = "build/build/Debug"
local COVERAGE_STAMP = DEBUG_BUILD_TREE .. "/.beez-coverage-configured"
local REPORTS_DIR = env_or("REPORTS_DIR", "report")
local FUZZER_TIME = env_or("FUZZER_TIME", "30")
local MIN_LINE_COVERAGE = env_or("MIN_LINE_COVERAGE", "85")
local FUZZER_BIN = DEBUG_BUILD_TREE .. "/fuzz/fuzz_lua_dsl"

local CXX_SOURCE_PATTERNS = {
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

local SRC_CPP_PATTERNS = {
    "src/**/*.cpp",
}

local SECURITY_SOURCE_PATTERNS = {
    "src/**/*.cpp",
    "src/**/*.hpp",
    "src/**/*.h",
    "include/**/*.cpp",
    "include/**/*.hpp",
    "include/**/*.h",
}

local CMAKE_FILE_PATTERNS = {
    "CMakeLists.txt",
    "src/CMakeLists.txt",
    "src/app/CMakeLists.txt",
    "src/cli/CMakeLists.txt",
    "src/core/CMakeLists.txt",
    "src/logging/CMakeLists.txt",
    "src/plugins/CMakeLists.txt",
    "src/plugins/lua/CMakeLists.txt",
    "src/plugins/shell/CMakeLists.txt",
    "tests/CMakeLists.txt",
    "tests/unit/CMakeLists.txt",
    "tests/integration/CMakeLists.txt",
    "tests/system/CMakeLists.txt",
    "tests/fuzz/CMakeLists.txt",
    "tests/performance/CMakeLists.txt",
}

local function run_per_file_success_cache(ctx, opts)
    local config = ctx.get_config()
    if config == nil then
        print(opts.log_prefix .. " missing step config")
        return 1
    end

    local patterns = opts.patterns or config.patterns
    local files = ctx.glob(patterns)
    if #files == 0 then
        print(opts.log_prefix .. " no files matched")
        return 0
    end

    local misses = ctx.get_cache_misses()
    if #misses > 0 then
        print(opts.log_prefix .. " re-checking from previous failures:")
        for _, entry in ipairs(misses) do
            print("  - " .. entry)
        end
    end

    local checked = 0
    local skipped = 0
    local failed = 0
    local prefix = opts.log_prefix
    local worker_prefix = opts.worker_prefix
    local pending_jobs = {}
    local pending_paths = {}

    for index, source_path in ipairs(files) do
        if ctx.file_success_cached(source_path) then
            print(prefix .. " skip (cached): " .. source_path)
            skipped = skipped + 1
        else
            print(prefix .. " checking: " .. source_path)
            checked = checked + 1

            local cmd = opts.command_fn(config, source_path)
            pending_jobs[#pending_jobs + 1] = ctx:spawn({
                cmd = cmd,
            })
            pending_paths[#pending_paths + 1] = source_path
        end
    end

    if #pending_jobs > 0 then
        for job_index, job in ipairs(pending_jobs) do
            local source_path = pending_paths[job_index]
            local result = ctx:wait(job, { exitCode = true })

            if result.exitCode ~= 0 then
                ctx.record_file_cache_miss(source_path)
                failed = failed + 1
            else
                ctx.cache_file_success(source_path)
            end
        end
    end

    print(prefix .. " summary: checked=" .. checked .. " skipped=" .. skipped .. " failed=" .. failed)

    if failed > 0 then
        return 1
    end
    return 0
end

-- ── Tasks ────────────────────────────────────────────────────────────────────

task("clean_cache", "rm -rf .cache")

task("clang_tidy", {
    { name = "qa:lint" },
})

task("format", {
    { name = "format:apply" },
})

task("debug", {
    { name = "configure:debug" },
    { name = "build:debug" },
})

task("fuzzer", {
    { name = "configure:fuzzer" },
    { name = "build:fuzzer" },
})

task("clean_reports", "rm -rf " .. REPORTS_DIR)

-- ── Configure + build ────────────────────────────────────────────────────────

step({
    name = "configure:setup",
    phase = "configure",
    scope = "code",
    input = {
        "conanfile.py",
        "CMakeLists.txt",
        "CMakePresets.json",
        "cmake/**",
        "conan/**",
        "src/**/CMakeLists.txt",
        "tests/**/CMakeLists.txt",
    },
    output = {
        BUILD_TREE .. "/compile_commands.json",
        BUILD_TREE .. "/build.ninja",
    },
    description = "Conan install + CMake configure (" .. BUILD_TYPE .. ")",
    run = "conan install . --output-folder=build --build=missing " ..
        "-s build_type=" .. BUILD_TYPE .. " -pr " .. CONAN_PROFILE .. " -pr:b " .. CONAN_PROFILE ..
        " && cmake --preset " .. CMAKE_PRESET .. " -DBUILD_TESTING=ON -DBUILD_CACHE=ON",
})

step({
    name = "build:compile",
    phase = "build",
    scope = "code",
    input = {
        "src/**/*.cpp",
        "include/**/*.hpp",
        "tests/**/*.cpp",
        "CMakeLists.txt",
        "src/**/CMakeLists.txt",
        "tests/**/CMakeLists.txt",
        BUILD_TREE .. "/build.ninja",
    },
    output = {
        BUILD_TREE .. "/bin/beez",
        BUILD_TREE .. "/tests/unit/beez_tests",
        BUILD_TREE .. "/tests/integration/beez_integration_tests",
        BUILD_TREE .. "/tests/system/beez_system_tests",
        BUILD_TREE .. "/tests/performance/beez_perf_tests",
    },
    description = "CMake build (app + all test binaries)",
    run = "cmake --build --preset " .. CMAKE_PRESET,
})

-- order("configure:setup", "build:compile")

-- ── Tests (ctest per suite, step cache on binaries + sources) ────────────────

local function make_test_step(stepName, scope, suite, ctest_args, binary, extra_inputs, report_marker)
    local inputs = {
        binary,
        "src/**/*.cpp",
        "include/**/*.hpp",
    }
    for _, pattern in ipairs(extra_inputs) do
        inputs[#inputs + 1] = pattern
    end

    step({
        name = stepName,
        phase = "test",
        scope = scope,
        input = inputs,
        output = { report_marker },
        description = "Run " .. suite .. " tests via ctest",
        run = "mkdir -p report/test && cd " .. BUILD_TREE .. " && ctest " .. ctest_args ..
            " --output-on-failure && touch ../../../" .. report_marker,
    })
end

make_test_step(
    "test:unit",
    "code",
    "unit",
    "-R beez_tests",
    BUILD_TREE .. "/tests/unit/beez_tests",
    { "tests/unit/**/*.cpp" },
    "report/test/unit.ok"
)

make_test_step(
    "test:integration",
    "code",
    "integration",
    "-R beez_integration_tests",
    BUILD_TREE .. "/tests/integration/beez_integration_tests",
    { "tests/integration/**/*.cpp" },
    "report/test/integration.ok"
)

make_test_step(
    "test:system",
    "code",
    "system",
    "-L system",
    BUILD_TREE .. "/tests/system/beez_system_tests",
    { "tests/system/**/*.cpp" },
    "report/test/system.ok"
)

make_test_step(
    "test:performance",
    "code",
    "performance",
    "-L performance",
    BUILD_TREE .. "/tests/performance/beez_perf_tests",
    { "tests/performance/**/*.cpp" },
    "report/test/performance.ok"
)

-- ── Format (check + apply) ───────────────────────────────────────────────────

configure_step("qa:format-check", {
    patterns = CXX_SOURCE_PATTERNS,
    cmake_patterns = CMAKE_FILE_PATTERNS,
    format_rev = "1",
})

step({
    name = "qa:format-check",
    phase = "qa",
    scope = "code",
    input = CXX_SOURCE_PATTERNS,
    description = "clang-format + cmake-format check (incremental)",
    run = function(ctx)
        local config = ctx.get_config()
        local clang_code = run_per_file_success_cache(ctx, {
            log_prefix = "[clang-format]",
            worker_prefix = "fmt_cpp_",
            patterns = config.patterns,
            command_fn = function(_, path)
                return "scripts/clang-format-one.sh " .. path
            end,
        })
        if clang_code ~= 0 then
            return clang_code
        end

        return run_per_file_success_cache(ctx, {
            log_prefix = "[cmake-format]",
            worker_prefix = "fmt_cmake_",
            patterns = config.cmake_patterns,
            command_fn = function(_, path)
                return "scripts/cmake-format-one.sh " .. path
            end,
        })
    end,
})

configure_step("format:apply", {
    patterns = CXX_SOURCE_PATTERNS,
    cmake_patterns = CMAKE_FILE_PATTERNS,
    format_rev = "1",
})

step({
    name = "format:apply",
    phase = "format",
    scope = "code",
    mutate = CXX_SOURCE_PATTERNS,
    description = "Apply clang-format + cmake-format (incremental)",
    run = function(ctx)
        local config = ctx.get_config()
        local clang_code = run_per_file_success_cache(ctx, {
            log_prefix = "[clang-format]",
            worker_prefix = "apply_cpp_",
            patterns = config.patterns,
            command_fn = function(_, path)
                return "scripts/clang-format-one.sh " .. path .. " --apply"
            end,
        })
        if clang_code ~= 0 then
            return clang_code
        end

        return run_per_file_success_cache(ctx, {
            log_prefix = "[cmake-format]",
            worker_prefix = "apply_cmake_",
            patterns = config.cmake_patterns,
            command_fn = function(_, path)
                return "scripts/cmake-format-one.sh " .. path .. " --apply"
            end,
        })
    end,
})

-- ── Lint (clang-tidy + cmake-format, like scripts/lint.sh) ───────────────────

configure_step("qa:lint", {
    compdb = BUILD_TREE,
    header_filter = "(src|include|tests)/.*",
    lint_rev = "2",
    patterns = CXX_SOURCE_PATTERNS,
    cmake_patterns = CMAKE_FILE_PATTERNS,
})

step({
    name = "qa:lint",
    phase = "qa",
    scope = "code",
    input = CXX_SOURCE_PATTERNS,
    description = "clang-tidy + cmake-format check (incremental)",
    run = function(ctx)
        local config = ctx.get_config()
        local tidy_code = run_per_file_success_cache(ctx, {
            log_prefix = "[clang-tidy]",
            worker_prefix = "tidy_",
            patterns = config.patterns,
            command_fn = function(cfg, path)
                return "scripts/clang-tidy-one.sh " .. cfg.compdb .. " " .. path ..
                    " '" .. cfg.header_filter .. "'"
            end,
        })
        if tidy_code ~= 0 then
            return tidy_code
        end

        return run_per_file_success_cache(ctx, {
            log_prefix = "[cmake-format]",
            worker_prefix = "lint_cmake_",
            patterns = config.cmake_patterns,
            command_fn = function(_, path)
                return "scripts/cmake-format-one.sh " .. path
            end,
        })
    end,
})

-- ── Static analysis ──────────────────────────────────────────────────────────

step({
    name = "qa:cppcheck-analyze",
    phase = "qa",
    scope = "code",
    input = {
        "src/**/*.cpp",
        "include/**/*.hpp",
    },
    output = { "report/analyze/cppcheck.ok" },
    description = "cppcheck on src/ (step cache)",
    run = "mkdir -p report/analyze && scripts/cppcheck-analyze.sh && touch report/analyze/cppcheck.ok",
})

configure_step("qa:analyze-tidy", {
    compdb = BUILD_TREE,
    header_filter = "(src|include|tests)/.*",
    analyze_rev = "1",
    patterns = SRC_CPP_PATTERNS,
})

step({
    name = "qa:analyze-tidy",
    phase = "qa",
    scope = "code",
    input = SRC_CPP_PATTERNS,
    description = "clang-tidy analyzer checks on src/ (incremental)",
    run = function(ctx)
        return run_per_file_success_cache(ctx, {
            log_prefix = "[analyze-tidy]",
            worker_prefix = "analyze_",
            command_fn = function(cfg, path)
                return "scripts/analyze-tidy-one.sh " .. cfg.compdb .. " " .. path ..
                    " '" .. cfg.header_filter .. "'"
            end,
        })
    end,
})

-- ── Security ─────────────────────────────────────────────────────────────────

step({
    name = "qa:cppcheck-security",
    phase = "qa",
    scope = "code",
    input = SECURITY_SOURCE_PATTERNS,
    output = { "report/security/cppcheck.ok" },
    description = "cppcheck security scan (step cache)",
    run = "mkdir -p report/security && scripts/cppcheck-security.sh && touch report/security/cppcheck.ok",
})

step({
    name = "qa:dependency-audit",
    phase = "qa",
    scope = "code",
    input = {
        "conanfile.py",
        "scripts/sbom-generate.sh",
        "scripts/conan-graph-to-cyclonedx.py",
        "scripts/dependency-audit.sh",
        "scripts/ci-conan-profile.sh",
        "conan/profiles/**",
    },
    output = {
        "report/security/dependency-audit.ok",
        "report/security/dependency-audit.txt",
        "report/sbom/cyclonedx.json",
        "report/sbom/conan-graph.json",
    },
    description = "Dependency vulnerability scan (OSV, Conan SBOM)",
    run = "scripts/dependency-audit.sh build report && touch report/security/dependency-audit.ok",
})

configure_step("qa:security-tidy", {
    compdb = BUILD_TREE,
    header_filter = "(src|include|tests)/.*",
    security_rev = "1",
    patterns = SECURITY_SOURCE_PATTERNS,
})

step({
    name = "qa:security-tidy",
    phase = "qa",
    scope = "code",
    input = SECURITY_SOURCE_PATTERNS,
    description = "clang-tidy security checks (incremental)",
    run = function(ctx)
        return run_per_file_success_cache(ctx, {
            log_prefix = "[security-tidy]",
            worker_prefix = "sec_",
            command_fn = function(cfg, path)
                return "scripts/security-tidy-one.sh " .. cfg.compdb .. " " .. path ..
                    " '" .. cfg.header_filter .. "'"
            end,
        })
    end,
})

-- ── Debug build ──────────────────────────────────────────────────────────────

step({
    name = "configure:debug",
    phase = "configure",
    scope = "debug",
    input = {
        "conanfile.py",
        "CMakeLists.txt",
        "CMakePresets.json",
        "cmake/**",
        "conan/**",
        "src/**/CMakeLists.txt",
        "tests/**/CMakeLists.txt",
    },
    output = {
        DEBUG_BUILD_TREE .. "/compile_commands.json",
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    description = "Conan install + CMake configure (Debug)",
    run = "conan install . --output-folder=build --build=missing " ..
        "-s build_type=Debug -pr " .. CONAN_PROFILE .. " -pr:b " .. CONAN_PROFILE ..
        " && cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_CACHE=ON",
})

step({
    name = "build:debug",
    phase = "build",
    scope = "debug",
    input = {
        "src/**/*.cpp",
        "include/**/*.hpp",
        "tests/**/*.cpp",
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    output = { DEBUG_BUILD_TREE .. "/bin/beez" },
    description = "CMake Debug build",
    run = "cmake --build --preset conan-debug",
})

-- ── Coverage ─────────────────────────────────────────────────────────────────

step({
    name = "configure:coverage",
    phase = "configure",
    scope = "coverage",
    input = {
        "conanfile.py",
        "CMakeLists.txt",
        "cmake/**",
        "src/**/CMakeLists.txt",
        "tests/**/CMakeLists.txt",
    },
    output = {
        DEBUG_BUILD_TREE .. "/compile_commands.json",
        DEBUG_BUILD_TREE .. "/build.ninja",
        COVERAGE_STAMP,
    },
    description = "CMake configure with coverage instrumentation",
    run = "conan install . --output-folder=build --build=missing " ..
        "-s build_type=Debug -pr " .. CONAN_PROFILE .. " -pr:b " .. CONAN_PROFILE ..
        " && cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_CACHE=ON -DBUILD_COVERAGE=ON " ..
        "-DBUILD_FUZZER=OFF -DENABLE_ASAN=OFF -DENABLE_UBSAN=OFF " ..
        "&& grep -qE 'BUILD_COVERAGE:(BOOL|UNINITIALIZED)=ON' " .. DEBUG_BUILD_TREE .. "/CMakeCache.txt " ..
        "&& touch " .. COVERAGE_STAMP,
})

step({
    name = "build:coverage",
    phase = "build",
    scope = "coverage",
    input = {
        "src/**/*.cpp",
        "include/**/*.hpp",
        "tests/**/*.cpp",
        DEBUG_BUILD_TREE .. "/build.ninja",
        COVERAGE_STAMP,
    },
    output = { DEBUG_BUILD_TREE .. "/tests/unit/beez_tests" },
    description = "Build Debug with coverage flags",
    run = "cmake --build --preset conan-debug",
})

step({
    name = "test:coverage",
    phase = "test",
    scope = "coverage",
    input = {
        DEBUG_BUILD_TREE .. "/tests/unit/beez_tests",
        "src/**/*.cpp",
        "tests/**/*.cpp",
        COVERAGE_STAMP,
    },
    output = {
        REPORTS_DIR .. "/test/coverage-test-report.ok",
        REPORTS_DIR .. "/test/coverage-test-report.txt",
        DEBUG_BUILD_TREE .. "/**/*.gcda",
    },
    description = "Run tests and capture coverage test report",
    run = "./scripts/coverage-test.sh build " .. REPORTS_DIR,
})

step({
    name = "report:coverage",
    phase = "report",
    scope = "coverage",
    input = {
        DEBUG_BUILD_TREE .. "/tests/unit/beez_tests",
        "src/**/*.cpp",
        REPORTS_DIR .. "/test/coverage-test-report.ok",
        DEBUG_BUILD_TREE .. "/**/*.gcda",
    },
    output = { REPORTS_DIR .. "/coverage/index.html" },
    description = "Generate HTML coverage report and enforce minimum line coverage (" ..
        MIN_LINE_COVERAGE .. "%)",
    run = "MIN_LINE_COVERAGE=" .. MIN_LINE_COVERAGE .. " ./scripts/coverage-report.sh build " ..
        REPORTS_DIR,
})

-- ── Sanitizer ────────────────────────────────────────────────────────────────

step({
    name = "configure:sanitize",
    phase = "configure",
    scope = "sanitize",
    input = {
        "conanfile.py",
        "CMakeLists.txt",
        "cmake/**",
        "src/**/CMakeLists.txt",
        "tests/**/CMakeLists.txt",
    },
    output = {
        DEBUG_BUILD_TREE .. "/compile_commands.json",
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    description = "CMake configure with ASan/UBSan",
    run = "conan install . --output-folder=build --build=missing " ..
        "-s build_type=Debug -pr " .. CONAN_PROFILE .. " -pr:b " .. CONAN_PROFILE ..
        " && cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_CACHE=ON " ..
        "&& cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_COVERAGE=OFF " ..
        "-DBUILD_FUZZER=OFF -DENABLE_ASAN=ON -DENABLE_UBSAN=ON " ..
        "&& rm -f " .. COVERAGE_STAMP,
})

step({
    name = "build:sanitize",
    phase = "build",
    scope = "sanitize",
    input = {
        "src/**/*.cpp",
        "include/**/*.hpp",
        "tests/**/*.cpp",
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    output = { DEBUG_BUILD_TREE .. "/tests/unit/beez_tests" },
    description = "Build Debug with sanitizers",
    run = "cmake --build --preset conan-debug",
})

step({
    name = "test:sanitize",
    phase = "test",
    scope = "sanitize",
    input = {
        DEBUG_BUILD_TREE .. "/tests/unit/beez_tests",
        "src/**/*.cpp",
        "tests/**/*.cpp",
    },
    output = { REPORTS_DIR .. "/sanitize/sanitize-report.ok" },
    description = "Run tests under ASan/UBSan",
    run = "mkdir -p " .. REPORTS_DIR .. "/sanitize && bash -o pipefail -c 'cd " .. DEBUG_BUILD_TREE ..
        " && ctest --output-on-failure 2>&1 | tee ../../../" .. REPORTS_DIR ..
        "/sanitize/sanitize-report.txt' && touch " .. REPORTS_DIR ..
        "/sanitize/sanitize-report.ok",
})

-- ── ThreadSanitizer ──────────────────────────────────────────────────────────

step({
    name = "configure:tsan",
    phase = "configure",
    scope = "tsan",
    input = {
        "conanfile.py",
        "CMakeLists.txt",
        "cmake/**",
        "src/**/CMakeLists.txt",
        "tests/**/CMakeLists.txt",
    },
    output = {
        DEBUG_BUILD_TREE .. "/compile_commands.json",
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    description = "CMake configure with ThreadSanitizer",
    run = "conan install . --output-folder=build --build=missing " ..
        "-s build_type=Debug -pr " .. CONAN_PROFILE .. " -pr:b " .. CONAN_PROFILE ..
        " && cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_CACHE=ON " ..
        "&& cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_COVERAGE=OFF " ..
        "-DBUILD_FUZZER=OFF -DENABLE_ASAN=OFF -DENABLE_UBSAN=OFF -DENABLE_TSAN=ON " ..
        "&& rm -f " .. COVERAGE_STAMP,
})

step({
    name = "build:tsan",
    phase = "build",
    scope = "tsan",
    input = {
        "src/**/*.cpp",
        "include/**/*.hpp",
        "tests/**/*.cpp",
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    output = { DEBUG_BUILD_TREE .. "/tests/unit/beez_tests" },
    description = "Build Debug with ThreadSanitizer",
    run = "cmake --build --preset conan-debug",
})

step({
    name = "test:tsan",
    phase = "test",
    scope = "tsan",
    input = {
        DEBUG_BUILD_TREE .. "/tests/unit/beez_tests",
        "src/**/*.cpp",
        "tests/**/*.cpp",
    },
    output = { REPORTS_DIR .. "/tsan/tsan-report.ok" },
    description = "Run tests under ThreadSanitizer",
    run = "mkdir -p " .. REPORTS_DIR .. "/tsan && bash -o pipefail -c 'cd " .. DEBUG_BUILD_TREE ..
        " && ctest --output-on-failure 2>&1 | tee ../../../" .. REPORTS_DIR ..
        "/tsan/tsan-report.txt' && touch " .. REPORTS_DIR .. "/tsan/tsan-report.ok",
})

-- ── Fuzzer ───────────────────────────────────────────────────────────────────

step({
    name = "configure:fuzzer",
    phase = "configure",
    scope = "fuzz",
    input = {
        "conanfile.py",
        "CMakeLists.txt",
        "tests/fuzz/**",
        "cmake/**",
    },
    output = {
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    description = "CMake configure for fuzzer target",
    run = "conan install . --output-folder=build --build=missing " ..
        "-s build_type=Debug -pr " .. CONAN_PROFILE .. " -pr:b " .. CONAN_PROFILE ..
        " && cmake --preset conan-debug -DBUILD_TESTING=OFF -DBUILD_CACHE=ON " ..
        "&& cmake --preset conan-debug -DBUILD_TESTING=OFF -DBUILD_COVERAGE=OFF " ..
        "-DBUILD_FUZZER=ON -DENABLE_ASAN=OFF -DENABLE_UBSAN=OFF",
})

step({
    name = "build:fuzzer",
    phase = "build",
    scope = "fuzz",
    input = {
        "tests/fuzz/**",
        DEBUG_BUILD_TREE .. "/build.ninja",
    },
    output = { FUZZER_BIN },
    description = "Build fuzz_lua_dsl",
    run = "cmake --build --preset conan-debug --target fuzz_lua_dsl",
})

step({
    name = "fuzz:smoke",
    phase = "fuzz",
    scope = "smoke",
    input = {
        FUZZER_BIN,
        "tests/fuzz/corpus/lua_dsl/*.lua",
        "tests/fuzz/lua_dsl.dict",
        "scripts/fuzz-common.sh",
        "scripts/fuzz-smoke.sh",
    },
    output = { REPORTS_DIR .. "/fuzz/fuzz-smoke-report.txt" },
    description = "Short fuzz run (" .. FUZZER_TIME .. "s default)",
    run = "FUZZER_TIME=" .. FUZZER_TIME .. " REPORTS_DIR=" .. REPORTS_DIR ..
        " scripts/fuzz-smoke.sh build",
})

step({
    name = "fuzz:corpus",
    phase = "fuzz",
    scope = "corpus",
    input = {
        FUZZER_BIN,
        "tests/fuzz/corpus/lua_dsl/*.lua",
        "tests/fuzz/lua_dsl.dict",
        "scripts/fuzz-common.sh",
        "scripts/fuzz-corpus.sh",
    },
    output = { REPORTS_DIR .. "/fuzz/fuzz-corpus-report.txt" },
    description = "Longer fuzz run for corpus collection (60s)",
    run = "FUZZER_TIME=60 REPORTS_DIR=" .. REPORTS_DIR .. " scripts/fuzz-corpus.sh build",
})

step({
    name = "fuzz:seed-verify",
    phase = "fuzz",
    scope = "verify",
    input = {
        FUZZER_BIN,
        "tests/fuzz/corpus/lua_dsl/*.lua",
        "scripts/fuzz-seed-verify.sh",
    },
    output = { REPORTS_DIR .. "/fuzz/fuzz-seed-verify-report.txt" },
    description = "Run each committed fuzz seed through the harness once",
    run = "REPORTS_DIR=" .. REPORTS_DIR .. " scripts/fuzz-seed-verify.sh build",
})

step({
    name = "fuzz:torture",
    phase = "fuzz",
    scope = "torture",
    input = {
        FUZZER_BIN,
        "tests/fuzz/corpus/lua_dsl/*.lua",
        "tests/fuzz/lua_dsl.dict",
        "scripts/fuzz-common.sh",
        "scripts/fuzz-torture.sh",
    },
    output = { REPORTS_DIR .. "/fuzz/fuzz-torture-report.txt" },
    description = "Aggressive multi-minute fuzz run (FUZZER_TIME=300 default)",
    run = "FUZZER_TIME=" .. (beez.env("FUZZER_TORTURE_TIME") or "300") ..
        " REPORTS_DIR=" .. REPORTS_DIR .. " scripts/fuzz-torture.sh build",
})

workflow("fuzzer_torture", {
    { phase = "configure", scope = "fuzz" },
    { phase = "build", scope = "fuzz" },
    { phase = "fuzz", scope = "torture" },
})

-- ── Clean ────────────────────────────────────────────────────────────────────

step({
    name = "clean:artifacts",
    phase = "clean",
    scope = "repo",
    description = "Remove build tree, reports, and caches",
    run = "rm -rf build " .. REPORTS_DIR .. " .cache",
})

-- ── Workflows ────────────────────────────────────────────────────────────────

workflow("build", {
    { phase = "configure", scope = "code" },
    { phase = "build", scope = "code" },
    { phase = "test", scope = "code" },
})

workflow("quality", {
    { phase = "qa", scope = "code" },
})

workflow("debug", {
    { phase = "configure", scope = "debug" },
    { phase = "build", scope = "debug" },
})

workflow("coverage", {
    { phase = "configure", scope = "coverage" },
    { phase = "build", scope = "coverage" },
    { phase = "test", scope = "coverage" },
    { phase = "report", scope = "coverage" },
})

workflow("sanitize", {
    { phase = "configure", scope = "sanitize" },
    { phase = "build", scope = "sanitize" },
    { phase = "test", scope = "sanitize" },
})

workflow("tsan", {
    { phase = "configure", scope = "tsan" },
    { phase = "build", scope = "tsan" },
    { phase = "test", scope = "tsan" },
})

workflow("fuzzer_smoke", {
    { phase = "configure", scope = "fuzz" },
    { phase = "build", scope = "fuzz" },
    { phase = "fuzz", scope = "smoke" },
})

workflow("fuzzer_corpus", {
    { phase = "configure", scope = "fuzz" },
    { phase = "build", scope = "fuzz" },
    { phase = "fuzz", scope = "corpus" },
})

workflow("all", {
    { phase = "configure", scope = "code" },
    { phase = "build", scope = "code" },
    { phase = "test", scope = "code" },
    { phase = "qa", scope = "code" },
    { phase = "configure", scope = "coverage" },
    { phase = "build", scope = "coverage" },
    { phase = "test", scope = "coverage" },
    { phase = "report", scope = "coverage" },
    { phase = "configure", scope = "sanitize" },
    { phase = "build", scope = "sanitize" },
    { phase = "test", scope = "sanitize" },
    { phase = "configure", scope = "fuzz" },
    { phase = "build", scope = "fuzz" },
    { phase = "fuzz", scope = "smoke" },
})

workflow("clean", {
    { phase = "clean", scope = "repo" },
})
