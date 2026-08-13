
reqpack {
    beez = {
        { name = "coditary/clang-format", path = "./plugins/coditary/clang-format", version = "1.0.0" },
        { name = "coditary/clang-tidy", path = "./plugins/coditary/clang-tidy", version = "1.0.0" },
        { name = "coditary/cppcheck", path = "./plugins/coditary/cppcheck", version = "1.0.0" },
        { name = "coditary/conan", path = "./plugins/coditary/conan", version = "1.0.0" },
        { name = "coditary/cyclonedx", path = "./plugins/coditary/cyclonedx", version = "1.0.0" },
        { name = "coditary/osv-audit", path = "./plugins/coditary/osv-audit", version = "1.0.0" },
        { name = "coditary/ctest", path = "./plugins/coditary/ctest", version = "1.0.0" },
        { name = "coditary/coverage", path = "./plugins/coditary/coverage", version = "1.0.0" },
        { name = "coditary/fuzzer", path = "./plugins/coditary/fuzzer", version = "1.0.0" },
        { name = "coditary/clang-build", path = "./plugins/coditary/clang-build", version = "1.0.0" },
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

configure({
    { "coditary/clang-tidy", {
        compdb = BUILD_TREE,
        check_rev = "2",
        lint_rev = "4",
        analyze_rev = "3",
        security_rev = "3",
        steps = {
            check = { profiles = { "lint", "analyze", "security" } },
        },
    }},

    { "coditary/cppcheck", {
        check_rev = "1",
        analyze_rev = "1",
        security_rev = "1",
        steps = {
            cppcheck_check = { profiles = { "analyze", "security" } },
        },
    }},

    { "coditary/conan", {
        reports_dir = REPORTS_DIR,
        lock_rev = "1",
        sbom_rev = "1",
    }},

    { "coditary/cyclonedx", {
        check_rev = "1",
        merge_rev = "1",
        merge_inputs = { REPORTS_DIR .. "/sbom/cyclonedx.json" },
    }},

    { "coditary/osv-audit", {
        audit_rev = "1",
    }},

    { "coditary/ctest", {
        reports_dir = REPORTS_DIR,
        test_rev = "1",
    }},

    { "coditary/coverage", {
        report_rev = "1",
    }},

    { "coditary/fuzzer", {
        fuzz_rev = "1",
    }},

    { "coditary/clang-build", {
        compile_rev = "1",
        link_rev = "1",
    }},
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
    { "configure", { "configure[fuzz]" } },
    { "build", { "build[fuzz]" } },
    { "fuzz", { "fuzz[torture]" } },
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
    { "configure", { "configure[code]" } },
    { "build", { "build[code]" } },
    { "test", { "test[code]" } },
})

workflow("quality", {
    { "code", { "qa[code]" } },
    { "supply", { "qa[supply]" } },
})

workflow("debug", {
    { "configure", { "configure[debug]" } },
    { "build", { "build[debug]" } },
})

workflow("coverage", {
    { "configure", { "configure[coverage]" } },
    { "build", { "build[coverage]" } },
    { "test", { "test[coverage]" } },
    { "report", { "report[coverage]" } },
})

workflow("sanitize", {
    { "configure", { "configure[sanitize]" } },
    { "build", { "build[sanitize]" } },
    { "test", { "test[sanitize]" } },
})

workflow("tsan", {
    { "configure", { "configure[tsan]" } },
    { "build", { "build[tsan]" } },
    { "test", { "test[tsan]" } },
})

workflow("fuzzer_smoke", {
    { "configure", { "configure[fuzz]" } },
    { "build", { "build[fuzz]" } },
    { "fuzz", { "fuzz[smoke]" } },
})

workflow("fuzzer_corpus", {
    { "configure", { "configure[fuzz]" } },
    { "build", { "build[fuzz]" } },
    { "fuzz", { "fuzz[corpus]" } },
})

workflow("all", {
    { "build", { "configure[code]", "build[code]", "test[code]" } },
    { "quality", { "qa[code]", "qa[supply]" } },
    { "coverage", { "configure[coverage]", "build[coverage]", "test[coverage]", "report[coverage]" } },
    { "sanitize", { "configure[sanitize]", "build[sanitize]", "test[sanitize]" } },
    { "fuzz", { "configure[fuzz]", "build[fuzz]", "fuzz[smoke]" } },
})

workflow("clean", {
    { "clean", { "clean[repo]" } },
})
