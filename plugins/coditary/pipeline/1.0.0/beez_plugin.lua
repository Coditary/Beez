-- Reusable staged workflows for Beez C++ projects.
-- Import in build.lua via workflows({ build = "coditary/pipeline:build", ... }).
-- All workflows use the standard stage names: setup, generate, quality, compile,
-- bundle, test, package, verify, publish.

plugin("pipeline", {
    version = "1.0.0",
    description = "Standard staged workflows (build, quality, coverage, fuzz, standard, …)",
    organization = "coditary",
    steps = {},
})

workflows {
    build = {
        { "setup", { "setup[code]" } },
        { "compile", { "compile[code]" } },
        { "bundle", { "bundle[code]" } },
        { "test", { "test[code]" } },
    },

    quality = {
        { "quality", { "quality[code]" } },
        { "package", { "package[supply]" } },
        { "verify", { "verify[supply]" } },
    },

    debug = {
        { "setup", { "setup[debug]" } },
        { "compile", { "compile[debug]" } },
        { "bundle", { "bundle[debug]" } },
    },

    coverage = {
        { "setup", { "setup[coverage]" } },
        { "compile", { "compile[coverage]" } },
        { "test", { "test[coverage]" } },
        { "package", { "package[coverage]" } },
    },

    sanitize = {
        { "setup", { "setup[sanitize]" } },
        { "compile", { "compile[sanitize]" } },
        { "test", { "test[sanitize]" } },
    },

    tsan = {
        { "setup", { "setup[tsan]" } },
        { "compile", { "compile[tsan]" } },
        { "test", { "test[tsan]" } },
    },

    fuzzer_smoke = {
        { "setup", { "setup[fuzz]" } },
        { "compile", { "compile[fuzz]" } },
        { "bundle", { "bundle[fuzz]" } },
        { "test", { "test[smoke]" } },
    },

    fuzzer_corpus = {
        { "setup", { "setup[fuzz]" } },
        { "compile", { "compile[fuzz]" } },
        { "bundle", { "bundle[fuzz]" } },
        { "test", { "test[corpus]" } },
    },

    fuzzer_torture = {
        { "setup", { "setup[fuzz]" } },
        { "compile", { "compile[fuzz]" } },
        { "bundle", { "bundle[fuzz]" } },
        { "test", { "test[torture]" } },
    },

    all = {
        { "setup", { "setup[code]", "setup[coverage]", "setup[sanitize]", "setup[fuzz]" } },
        { "compile", { "compile[code]", "compile[coverage]", "compile[sanitize]", "compile[fuzz]" } },
        { "bundle", { "bundle[code]", "bundle[coverage]", "bundle[sanitize]", "bundle[fuzz]" } },
        { "test", { "test[code]", "test[coverage]", "test[sanitize]", "test[smoke]" } },
        { "quality", { "quality[code]" } },
        { "package", { "package[supply]", "package[coverage]" } },
        { "verify", { "verify[supply]" } },
    },

    clean = {
        { "setup", { "setup[repo]" } },
    },

    standard = {
        { "setup", { "setup" } },
        { "generate", { "generate" } },
        { "quality", { "quality" } },
        { "compile", { "compile" } },
        { "bundle", { "bundle" } },
        { "test", { "test" } },
        { "package", { "package" } },
        { "verify", { "verify" } },
        { "publish", { "publish" } },
    },
}
