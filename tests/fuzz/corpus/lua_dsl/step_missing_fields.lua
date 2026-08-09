step({
    phase = "generate",
    scope = "docs",
    run = "true",
})
step({
    name = "no-phase",
    scope = "code",
    run = "true",
})
step({
    name = "no-scope",
    phase = "compile",
})
step({
    name = "no-run",
    phase = "compile",
    scope = "code",
})
