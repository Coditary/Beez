step({
    name = "gen-docs",
    phase = "generate",
    scope = "docs",
    run = "echo docs > docs.out",
})
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "echo code > code.out",
})
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = "echo built > build.out",
})
