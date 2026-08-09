step({
    name = "gen-docs",
    phase = "generate",
    scope = "docs",
    description = "Generate documentation",
    run = "echo unicode-step",
})
task("über-build", "echo unicode-task")
