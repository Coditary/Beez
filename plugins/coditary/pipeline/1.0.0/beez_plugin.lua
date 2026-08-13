-- Reusable staged workflows for Beez C++ projects.
-- Import in build.lua via workflows({ build = "coditary/pipeline:build", ... }).

plugin("pipeline", {
    version = "1.0.0",
    description = "Standard staged workflows (build, quality, coverage, fuzz, …)",
    organization = "coditary",
    steps = {},
})

workflows {
    build = {
        { "configure", { "configure[code]" } },
        { "build", { "build[code]" } },
        { "test", { "test[code]" } },
    },

    quality = {
        { "code", { "qa[code]" } },
        { "supply", { "qa[supply]" } },
    },

    debug = {
        { "configure", { "configure[debug]" } },
        { "build", { "build[debug]" } },
    },

    coverage = {
        { "configure", { "configure[coverage]" } },
        { "build", { "build[coverage]" } },
        { "test", { "test[coverage]" } },
        { "report", { "report[coverage]" } },
    },

    sanitize = {
        { "configure", { "configure[sanitize]" } },
        { "build", { "build[sanitize]" } },
        { "test", { "test[sanitize]" } },
    },

    tsan = {
        { "configure", { "configure[tsan]" } },
        { "build", { "build[tsan]" } },
        { "test", { "test[tsan]" } },
    },

    fuzzer_smoke = {
        { "configure", { "configure[fuzz]" } },
        { "build", { "build[fuzz]" } },
        { "fuzz", { "fuzz[smoke]" } },
    },

    fuzzer_corpus = {
        { "configure", { "configure[fuzz]" } },
        { "build", { "build[fuzz]" } },
        { "fuzz", { "fuzz[corpus]" } },
    },

    fuzzer_torture = {
        { "configure", { "configure[fuzz]" } },
        { "build", { "build[fuzz]" } },
        { "fuzz", { "fuzz[torture]" } },
    },

    all = {
        { "build", { "configure[code]", "build[code]", "test[code]" } },
        { "quality", { "qa[code]", "qa[supply]" } },
        { "coverage", { "configure[coverage]", "build[coverage]", "test[coverage]", "report[coverage]" } },
        { "sanitize", { "configure[sanitize]", "build[sanitize]", "test[sanitize]" } },
        { "fuzz", { "configure[fuzz]", "build[fuzz]", "fuzz[smoke]" } },
    },

    clean = {
        { "clean", { "clean[repo]" } },
    },
}
