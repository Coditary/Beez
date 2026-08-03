task("gen-docs", { phase = "generate", scope = "docs", run = "echo docs > wf-docs.out" })
task("gen-code", { phase = "generate", scope = "code", run = "echo gen > gen.out" })
task("compile", { phase = "compile", scope = "code", run = "echo compiled > compile.out" })

workflow("build", {
    { phase = "generate", scope = "code" },
    { phase = "compile", scope = "code" },
})

workflow("ci", {
    { parallel = {
        { phase = "generate", scope = "docs" },
        { phase = "generate", scope = "code" },
    }},
    { phase = "compile", scope = "code" },
})
