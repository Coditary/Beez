step({ name = "setup-default", phase = "setup", scope = "default", run = "true" })
workflow("standard", {
    { "setup", { "setup" } },
    { "compile", { "compile" } },
})
