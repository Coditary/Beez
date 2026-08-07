-- Beez — real project pipeline (repo root):
--
--   beez build              Conan + CMake configure, compile, run all test suites
--   beez quality            format-check, clang-tidy lint, analyze, security
--   beez clean              remove build/, report/, .cache/
--   beez format             apply clang-format + cmake-format (incremental)
--   beez clang_tidy         clang-tidy only (same as qa:lint step)
--   beez clean_cache        clear .cache/ only
--
-- Incremental QA uses per-file success cache (.cache/success/).
-- Build/test steps use step cache (.cache/) keyed on inputs/outputs.
-- Bump lint_rev / analyze_rev / security_rev in configure_step after toolchain changes.
-- BUILD_TYPE / CONAN_PROFILE: process env or .env (default Release / clang-release).

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

    for index, source_path in ipairs(files) do
        if ctx.file_success_cached(source_path) then
            print(prefix .. " skip (cached): " .. source_path)
            skipped = skipped + 1
        else
            print(prefix .. " checking: " .. source_path)
            checked = checked + 1

            local cmd = opts.command_fn(config, source_path)
            local job = ctx:spawn({
                name = worker_prefix .. index,
                cmd = cmd,
            })
            local code = ctx:wait(job)

            if code ~= 0 then
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

-- ── Configure + build ────────────────────────────────────────────────────────

step({
    name = "configure:setup",
    phase = "configure",
    scope = "project",
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
    scope = "project",
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

order("configure:setup", "build:compile")

-- ── Tests (ctest per suite, step cache on binaries + sources) ────────────────

local function make_test_step(name, scope, ctest_args, binary, extra_inputs, report_marker)
    local inputs = {
        binary,
        "src/**/*.cpp",
        "include/**/*.hpp",
    }
    for _, pattern in ipairs(extra_inputs) do
        inputs[#inputs + 1] = pattern
    end

    step({
        name = name,
        phase = "test",
        scope = scope,
        input = inputs,
        output = { report_marker },
        description = "Run " .. scope .. " tests via ctest",
        run = "mkdir -p report/test && cd " .. BUILD_TREE .. " && ctest " .. ctest_args ..
            " --output-on-failure && touch ../../../" .. report_marker,
    })
end

make_test_step(
    "test:unit",
    "unit",
    "-R beez_tests",
    BUILD_TREE .. "/tests/unit/beez_tests",
    { "tests/unit/**/*.cpp" },
    "report/test/unit.ok"
)

make_test_step(
    "test:integration",
    "integration",
    "-R beez_integration_tests",
    BUILD_TREE .. "/tests/integration/beez_integration_tests",
    { "tests/integration/**/*.cpp" },
    "report/test/integration.ok"
)

make_test_step(
    "test:system",
    "system",
    "-L system",
    BUILD_TREE .. "/tests/system/beez_system_tests",
    { "tests/system/**/*.cpp" },
    "report/test/system.ok"
)

make_test_step(
    "test:performance",
    "performance",
    "-L performance",
    BUILD_TREE .. "/tests/performance/beez_perf_tests",
    { "tests/performance/**/*.cpp" },
    "report/test/performance.ok"
)

order("build:compile", "test:unit")
order("build:compile", "test:integration")
order("build:compile", "test:system")
order("build:compile", "test:performance")

-- ── Format (check + apply) ───────────────────────────────────────────────────

configure_step("qa:format-check", {
    patterns = CXX_SOURCE_PATTERNS,
    cmake_patterns = CMAKE_FILE_PATTERNS,
    format_rev = "1",
})

step({
    name = "qa:format-check",
    phase = "qa",
    scope = "format",
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
    scope = "project",
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
    scope = "lint",
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
    scope = "analyze",
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
    scope = "analyze",
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

order("qa:cppcheck-analyze", "qa:analyze-tidy")

-- ── Security ─────────────────────────────────────────────────────────────────

step({
    name = "qa:cppcheck-security",
    phase = "qa",
    scope = "security",
    input = SECURITY_SOURCE_PATTERNS,
    output = { "report/security/cppcheck.ok" },
    description = "cppcheck security scan (step cache)",
    run = "mkdir -p report/security && scripts/cppcheck-security.sh && touch report/security/cppcheck.ok",
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
    scope = "security",
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

order("qa:cppcheck-security", "qa:security-tidy")

-- ── Clean ────────────────────────────────────────────────────────────────────

step({
    name = "clean:artifacts",
    phase = "clean",
    scope = "project",
    description = "Remove build tree, reports, and caches",
    run = "rm -rf build report .cache",
})

-- ── Workflows ────────────────────────────────────────────────────────────────

workflow("build", {
    { phase = "configure", scope = "project" },
    { phase = "build", scope = "project" },
    { phase = "test", scope = "unit" },
    { phase = "test", scope = "integration" },
    { phase = "test", scope = "system" },
    { phase = "test", scope = "performance" },
})

workflow("quality", {
    { phase = "qa", scope = "format" },
    { phase = "qa", scope = "lint" },
    { phase = "qa", scope = "analyze" },
    { phase = "qa", scope = "security" },
})

workflow("clean", {
    { phase = "clean", scope = "project" },
})
