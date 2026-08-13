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
-- order() accepts a chain: order("a", "b", "c") means a → b → c.

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
        {
            name = "coditary/fuzzer",
            path = "./plugins/coditary/fuzzer",
            version = "1.0.0",
        },
        {
            name = "coditary/clang-build",
            path = "./plugins/coditary/clang-build",
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
local REPORTS_DIR = env_or("REPORTS_DIR", "report")

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

local function order_build_chain(scope)
    local configure_step = scope == "code" and "configure:setup" or ("configure:" .. scope)
    order(configure_step, "compile:" .. scope, "link:" .. scope)
end

-- order(
--     "conan_lock_create",
--     "conan_sbom_export",
--     "cyclonedx_check",
--     "cyclonedx_merge",
--     "osv_audit_check"
-- )


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

local function configure_fuzz(step_name, extra)
    if extra ~= nil then
        configure_step(step_name, extra)
    end
end

configure_fuzz("fuzz:smoke", { fuzz_rev = "1" })
configure_fuzz("fuzz:corpus", { fuzz_rev = "1" })
configure_fuzz("fuzz:seed-verify", { fuzz_rev = "1" })
configure_fuzz("fuzz:torture", { fuzz_rev = "1" })

local function configure_clang_build(step_name, extra)
    if extra ~= nil then
        configure_step(step_name, extra)
    end
end

configure_clang_build("compile:code", { compile_rev = "1" })
configure_clang_build("link:code", { link_rev = "1" })
configure_clang_build("compile:debug", { compile_rev = "1" })
configure_clang_build("link:debug", { link_rev = "1" })
configure_clang_build("compile:coverage", { compile_rev = "1" })
configure_clang_build("link:coverage", { link_rev = "1" })
configure_clang_build("compile:sanitize", { compile_rev = "1" })
configure_clang_build("link:sanitize", { link_rev = "1" })
configure_clang_build("compile:tsan", { compile_rev = "1" })
configure_clang_build("link:tsan", { link_rev = "1" })
configure_clang_build("compile:fuzzer", { compile_rev = "1" })
configure_clang_build("link:fuzzer", { link_rev = "1" })

-- order("test:unit", "test:integration", "test:system", "test:performance")

-- ── Tasks ────────────────────────────────────────────────────────────────────

task("clean_cache", "rm -rf .cache")

task("debug", {
    { plugin = "coditary/conan", step = "configure", scope = "debug" },
    { plugin = "coditary/clang-build", step = "compile", scope = "debug" },
    { plugin = "coditary/clang-build", step = "link", scope = "debug" },
})

task("fuzzer", {
    { plugin = "coditary/conan", step = "configure", scope = "fuzzer" },
    { plugin = "coditary/clang-build", step = "compile", scope = "fuzzer" },
    { plugin = "coditary/clang-build", step = "link", scope = "fuzzer" },
})

task("clean_reports", "rm -rf " .. REPORTS_DIR)

-- ── Configure + build (coditary/conan configure + coditary/clang-build) ────────

-- ── Supply chain (coditary/conan + cyclonedx + osv-audit plugins) ────────────

-- ── Debug build (coditary/conan plugin) ──────────────────────────────────────

-- ── Coverage (coditary/conan + coditary/ctest + coditary/coverage plugins) ───

-- ── Sanitizer / TSan reports (coditary/ctest plugin runs tests) ─────────────

-- ── Fuzzer (coditary/fuzzer plugin; configure/build via coditary/conan) ────────

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
