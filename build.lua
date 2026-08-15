
reqpack {
    beez = {
        { name = "coditary/clang-format", path = "./plugins/coditary/clang-format", version = "1.0.0" },
        { name = "coditary/clang-tidy",   path = "./plugins/coditary/clang-tidy",   version = "1.0.0" },
        { name = "coditary/cppcheck",     path = "./plugins/coditary/cppcheck",     version = "1.0.0" },
        { name = "coditary/conan",        path = "./plugins/coditary/conan",        version = "1.0.0" },
        { name = "coditary/cyclonedx",    path = "./plugins/coditary/cyclonedx",    version = "1.0.0" },
        { name = "coditary/osv-audit",    path = "./plugins/coditary/osv-audit",    version = "1.0.0" },
        { name = "coditary/ctest",        path = "./plugins/coditary/ctest",        version = "1.0.0" },
        { name = "coditary/coverage",     path = "./plugins/coditary/coverage",     version = "1.0.0" },
        { name = "coditary/fuzzer",       path = "./plugins/coditary/fuzzer",       version = "1.0.0" },
        { name = "coditary/clang-build",  path = "./plugins/coditary/clang-build",  version = "1.0.0" },
        { name = "coditary/pipeline",     path = "./plugins/coditary/pipeline",     version = "1.0.0" },
    },
}

beez.config(require("config"))

local BUILD_TYPE = beez.env_or("BUILD_TYPE", "Release")
local BUILD_TREE = "build/build/" .. BUILD_TYPE
local REPORTS_DIR = beez.env_or("REPORTS_DIR", "report")
local OSV_SCANNER = beez.env_or("OSV_SCANNER", beez.env_or("HOME", "") .. "/.local/bin/osv-scanner")

-- ── Plugin + step configuration ──────────────────────────────────────────────

configure({
    { "coditary/clang-tidy", { compdb = BUILD_TREE, check_rev = "2", lint_rev = "6", analyze_rev = "3", security_rev = "3",
		steps = { check = { profiles = { "lint", "analyze", "security" } } } } },

    { "coditary/cppcheck", { check_rev = "2", analyze_rev = "3", security_rev = "3",
		steps = { cppcheck_check = { profiles = { "analyze", "security" } } } } },

    { "coditary/conan", { reports_dir = REPORTS_DIR, lock_rev = "1", sbom_rev = "1" } },
    { "coditary/cyclonedx", { check_rev = "1", merge_rev = "1", merge_inputs = { REPORTS_DIR .. "/sbom/cyclonedx.json" } } },
    { "coditary/osv-audit", { audit_rev = "1", osv_scanner = OSV_SCANNER } },
    { "coditary/ctest", { reports_dir = REPORTS_DIR, test_rev = "4" } },
    { "coditary/coverage", { report_rev = "1" } },
    { "coditary/fuzzer", { fuzz_rev = "4" } },
    { "coditary/clang-build", { compile_rev = "1", link_rev = "5" } },
})

-- ── Tasks ────────────────────────────────────────────────────────────────────

task("debug", {
    { plugin = "coditary/conan", step = "configure[debug]" },
    { plugin = "coditary/clang-build", step = "compile[debug]" },
    { plugin = "coditary/clang-build", step = "link[debug]" },
})

task("fuzzer", {
    { plugin = "coditary/conan", step = "configure[fuzzer]" },
    { plugin = "coditary/clang-build", step = "compile[fuzzer]" },
    { plugin = "coditary/clang-build", step = "link[fuzzer]" },
})

task("clean_reports", "rm -rf " .. REPORTS_DIR)

-- ── Clean ────────────────────────────────────────────────────────────────────

step({
    name = "clean:artifacts",
    phase = "setup",
    scope = "repo",
    description = "Remove build tree, reports, and caches",
    run = "rm -rf build " .. REPORTS_DIR .. " .cache",
})

-- ── Workflows (imported from coditary/pipeline) ──────────────────────────────

workflows({
    build = "coditary/pipeline:build",
    quality = "coditary/pipeline:quality",
    debug = "coditary/pipeline:debug",
    coverage = "coditary/pipeline:coverage",
    sanitize = "coditary/pipeline:sanitize",
    tsan = "coditary/pipeline:tsan",
    fuzzer_smoke = "coditary/pipeline:fuzzer_smoke",
    fuzzer_corpus = "coditary/pipeline:fuzzer_corpus",
    fuzzer_torture = "coditary/pipeline:fuzzer_torture",
    all = "coditary/pipeline:all",
    clean = "coditary/pipeline:clean",
})
