
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

-- ── Plugin + step configuration ──────────────────────────────────────────────
-- Shared settings per plugin via configure_plugin (merged into every step of that plugin).
-- configure_step only when a single step needs different behavior (e.g. multi-profile check).

configure_plugin("coditary/clang-tidy", {
    compdb = BUILD_TREE,
    check_rev = "2",
    lint_rev = "4",
    analyze_rev = "3",
    security_rev = "3",
})

configure_step("check", {
    profiles = { "lint", "analyze", "security" },
})

configure_plugin("coditary/cppcheck", {
    check_rev = "1",
    analyze_rev = "1",
    security_rev = "1",
})

configure_step("cppcheck_check", {
    profiles = { "analyze", "security" },
})

configure_plugin("coditary/conan", {
    reports_dir = REPORTS_DIR,
    lock_rev = "1",
    sbom_rev = "1",
})

configure_plugin("coditary/cyclonedx", {
    check_rev = "1",
    merge_rev = "1",
    merge_inputs = { REPORTS_DIR .. "/sbom/cyclonedx.json" },
})

configure_plugin("coditary/osv-audit", {
    audit_rev = "1",
})

configure_plugin("coditary/ctest", {
    reports_dir = REPORTS_DIR,
    test_rev = "1",
})

configure_plugin("coditary/coverage", {
    report_rev = "1",
})

configure_plugin("coditary/fuzzer", {
    fuzz_rev = "1",
})

configure_plugin("coditary/clang-build", {
    compile_rev = "1",
    link_rev = "1",
})

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
