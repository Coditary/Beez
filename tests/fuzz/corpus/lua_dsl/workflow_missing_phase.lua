step({
    name = "docs",
    phase = "generate",
    scope = "docs",
    run = "true",
})
workflow("build", {
    { scope = "docs" },
})
