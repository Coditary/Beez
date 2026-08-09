task("valid", "true")
step({
    name = "broken",
    phase = "generate",
    scope = "docs",
    run = 42,
})
beez.config({
    ui = {
        output_mode = "invalid-mode",
    },
})
workflow("bad", {
    { not_a_phase = "generate" },
})
task("also-broken", { phase = "generate", scope = "code", run = "true" })
