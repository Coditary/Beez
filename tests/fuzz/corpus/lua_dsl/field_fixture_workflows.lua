step({
    name = "gen-docs",
    phase = "generate",
    scope = "docs",
    run = "echo docs > wf-docs.out",
})
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "echo gen > gen.out",
})
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = "echo compiled > compile.out",
})

workflow("build", {
    { phase = "generate", scope = "code" },
    { phase = "compile", scope = "code" },
})

workflow("ci", {
    { phase = "generate", scope = "docs" },
    { phase = "generate", scope = "code" },
    { phase = "compile", scope = "code" },
})
