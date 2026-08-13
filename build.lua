-- Beez — real project pipeline (repo root):
--
--   beez build              Conan + CMake configure, compile, run all test suites
--   beez quality            qa phase (clang-format + clang-tidy plugins, cppcheck, …)
--   beez all                full QS pipeline (build + quality + coverage + sanitize + fuzz)
--   beez -s format_apply              incremental clang-format apply (coditary/clang-format plugin)
--   beez -s qa_check                  incremental clang-format check (plugin)
--   make format             clang-format + cmake-format (Makefile, not Beez)
--   make format-check       CI format check (Makefile)
--   beez -s check                      clang-tidy (profiles or custom checks array)
--   beez -s lint_check                 clang-tidy lint profile only
--   beez -s analyze_check              clang-tidy analyzer profile only
--   beez -s cppcheck_check              cppcheck (profiles: analyze, security)
--   beez -s cppcheck_analyze_check      cppcheck on src/
--   beez -s cppcheck_security_check     cppcheck security scan
--   beez -s conan_sbom_export             Conan graph + CycloneDX SBOM
--   beez -s cyclonedx_merge               merge CycloneDX BOM files
--   beez -s osv_audit_check               OSV vulnerability scan (lockfile)
--   beez -s test:unit                     unit tests (coditary/ctest plugin)
--   make lint                          clang-tidy + cmake-format (Makefile, not Beez)
--   beez clean_cache        clear .cache/ only
--
-- Incremental QA uses per-file success cache (.cache/success/).
-- Build/test steps use step cache (.cache/) keyed on inputs/outputs.
-- Bump check_rev / lint_rev / analyze_rev / security_rev after toolchain changes.
-- BUILD_TYPE / CONAN_PROFILE: process env or .env (default Release / clang-release).
--
-- Scopes group steps for workflow/CLI selection (phase + scope), not file domains.
--   code     — programming: configure (Release), build, test, qa
--   debug    — Debug configure + build (isolated from Release configure)
--   coverage / sanitize / tsan / fuzz — specialized code toolchains (own configure tree)
--   smoke / corpus         — fuzz run variants (phase fuzz, separate workflow targets)
--   repo                   — repository-wide clean (future: book, docs, …)
-- Steps in the same phase+scope are ordered via order(); independent steps run in parallel.

reqpack {
    beez = {
        {
            name = "coditary/clang-format",
            path = "./plugins/coditary/clang-format",
            version = "1.0.0",
        },
        {
            name = "coditary/clang-tidy",
            path = "./plugins/coditary/clang-tidy",
            version = "1.0.0",
        },
        {
            name = "coditary/cppcheck",
            path = "./plugins/coditary/cppcheck",
            version = "1.0.0",
        },
        {
            name = "coditary/conan",
            path = "./plugins/coditary/conan",
            version = "1.0.0",
        },
        {
            name = "coditary/cyclonedx",
            path = "./plugins/coditary/cyclonedx",
            version = "1.0.0",
        },
        {
            name = "coditary/osv-audit",
            path = "./plugins/coditary/osv-audit",
            version = "1.0.0",
        },
        {
            name = "coditary/ctest",
            path = "./plugins/coditary/ctest",
            version = "1.0.0",
        },
        {
            name = "coditary/coverage",
            path = "./plugins/coditary/coverage",
            version = "1.0.0",
        },
    },
}

beez.config(require("config"))

local function env_or(key, default)
    local value = beez.env(key)
    if value == nil then
        return default
    end
    return value
end

local BUILD_TYPE = env_or("BUILD_TYPE", "Release")
local BUILD_TREE = "build/build/" .. BUILD_TYPE
local DEBUG_BUILD_TREE = "build/build/Debug"
local REPORTS_DIR = env_or("REPORTS_DIR", "report")
local FUZZER_TIME = env_or("FUZZER_TIME", "30")
local FUZZER_BIN = DEBUG_BUILD_TREE .. "/fuzz/fuzz_lua_dsl"

local function configure_clang_tidy(step_name, extra)
    local cfg = { compdb = BUILD_TREE }
    if extra ~= nil then
        for key, value in pairs(extra) do
            cfg[key] = value
        end
    end
    configure_step(step_name, cfg)
end

configure_clang_tidy("check", {
    profiles = { "lint", "analyze", "security" },
    check_rev = "2",
})

configure_clang_tidy("lint_check", {
    lint_rev = "4",
})

configure_clang_tidy("analyze_check", {
    analyze_rev = "3",
})

configure_clang_tidy("security_check", {
    security_rev = "3",
})

local function configure_cppcheck(step_name, extra)
    local cfg = {}
    if extra ~= nil then
        for key, value in pairs(extra) do
            cfg[key] = value
        end
    end
    configure_step(step_name, cfg)
end

configure_cppcheck("cppcheck_check", {
    profiles = { "analyze", "security" },
    check_rev = "1",
})

configure_cppcheck("cppcheck_analyze_check", {
    analyze_rev = "1",
})

configure_cppcheck("cppcheck_security_check", {
    security_rev = "1",
})

local function configure_supply(step_name, extra)
    if extra ~= nil then
        configure_step(step_name, extra)
    end
end

configure_supply("conan_lock_create", {
    lock_rev = "1",
})

configure_supply("conan_sbom_export", {
    sbom_rev = "1",
})

configure_supply("cyclonedx_check", {
    check_rev = "1",
})

configure_supply("cyclonedx_merge", {
    merge_rev = "1",
    merge_inputs = { REPORTS_DIR .. "/sbom/cyclonedx.json" },
})

configure_supply("osv_audit_check", {
    audit_rev = "1",
})

order("conan_lock_create", "conan_sbom_export")
order("conan_sbom_export", "cyclonedx_check")
order("cyclonedx_check", "cyclonedx_merge")
order("cyclonedx_merge", "osv_audit_check")

order("configure:setup", "build:compile")

local function configure_test(step_name, extra)
    if extra ~= nil then
        configure_step(step_name, extra)
    end
end

configure_test("test:unit", { test_rev = "1" })
configure_test("test:integration", { test_rev = "1" })
configure_test("test:system", { test_rev = "1" })
configure_test("test:performance", { test_rev = "1" })
configure_test("test:coverage", { test_rev = "1" })

configure_step("report:coverage", { report_rev = "1" })
configure_test("test:sanitize", { test_rev = "1" })
configure_test("test:tsan", { test_rev = "1" })
configure_test("test:robustness", { test_rev = "1" })

order("test:unit", "test:integration")
order("test:integration", "test:system")
order("test:system", "test:performance")

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
        local results = ctx:wait_all(pending_jobs, { exitCode = true, duration = true })
        for job_index, result in ipairs(results) do
            local source_path = pending_paths[job_index]

            if result.exitCode ~= 0 then
                ctx.record_file_cache_miss(source_path)
                failed = failed + 1
            else
                ctx.cache_file_success(source_path, result.duration)
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

task("debug", {
    { name = "configure:debug" },
    { name = "build:debug" },
})

task("fuzzer", {
    { name = "configure:fuzzer" },
    { name = "build:fuzzer" },
})

task("clean_reports", "rm -rf " .. REPORTS_DIR)

-- ── Configure + build + tests (coditary/conan + coditary/ctest plugins) ────────

-- ── cmake-format (clang-tidy via coditary/clang-tidy plugin) ────────────────

configure_step("qa_cmake_check", {
    cmake_patterns = CMAKE_FILE_PATTERNS,
    format_rev = "1",
})

step({
    name = "qa_cmake_check",
    phase = "qa",
    scope = "code",
    input = CMAKE_FILE_PATTERNS,
    description = "cmake-format check (incremental)",
    run = function(ctx)
        local config = ctx.get_config()

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

-- ── Supply chain (coditary/conan + cyclonedx + osv-audit plugins) ────────────

-- ── Debug build (coditary/conan plugin) ──────────────────────────────────────

-- ── Coverage (coditary/conan + coditary/ctest + coditary/coverage plugins) ───

-- ── Sanitizer / TSan reports (coditary/ctest plugin runs tests) ─────────────

-- ── Fuzzer ───────────────────────────────────────────────────────────────────

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
    { phase = "qa", scope = "supply" },
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
    { phase = "qa", scope = "supply" },
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
