-- Reusable staged workflows for Beez C++ projects.
-- Import in build.lua via workflows({ build = "coditary/pipeline:build", ... }).
-- Standard phases: setup, generate, quality, compile, bundle, test, package, verify, publish
-- Standard scopes: app, debug, coverage, sanitize, tsan, fuzz, test, lint, format, analyze,
--                  security, audit, docs, code, repo

plugin("pipeline", {
    version = "1.0.0",
    description = "Standard staged workflows (build, quality, coverage, fuzz, standard, …)",
    organization = "coditary",
    steps = {},
})

workflows {
    build = {
        { "setup", { "setup[app]" } },
        { "compile", { "compile[app]" } },
        { "bundle", { "bundle[app]" } },
        { "test", { "test[test]" } },
    },

    quality = {
        { "quality", { "quality[lint]", "quality[format]", "quality[analyze]" } },
        { "package", { "package[audit]" } },
        { "verify", { "verify[security]", "verify[audit]" } },
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
        { "test", { "test[fuzz]" } },
    },

    fuzzer_corpus = {
        { "setup", { "setup[fuzz]" } },
        { "compile", { "compile[fuzz]" } },
        { "bundle", { "bundle[fuzz]" } },
        { "test", { "test[fuzz]" } },
    },

    fuzzer_torture = {
        { "setup", { "setup[fuzz]" } },
        { "compile", { "compile[fuzz]" } },
        { "bundle", { "bundle[fuzz]" } },
        { "test", { "test[fuzz]" } },
    },

    all = {
        { "setup", { "setup[app]", "setup[coverage]", "setup[sanitize]", "setup[fuzz]" } },
        { "compile", { "compile[app]", "compile[coverage]", "compile[sanitize]", "compile[fuzz]" } },
        { "bundle", { "bundle[app]", "bundle[coverage]", "bundle[sanitize]", "bundle[fuzz]" } },
        { "test", { "test[test]", "test[coverage]", "test[sanitize]", "test[fuzz]" } },
        { "quality", { "quality[lint]", "quality[format]", "quality[analyze]" } },
        { "verify", { "verify[security]", "verify[audit]", "verify[fuzz]" } },
        { "package", { "package[audit]", "package[coverage]" } },
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
