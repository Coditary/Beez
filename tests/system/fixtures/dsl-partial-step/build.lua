step({
    name = "gen",
    phase = "generate",
    scope = "docs",
})

step({
    name = "build",
    phase = "compile",
    scope = "code",
    run = "true",
})

configure_step("gen", {
    docs = true,
})

order("gen", "build")

workflow("ci", {
    { phase = "generate", scope = "docs" },
    { phase = "compile", scope = "code" },
})
