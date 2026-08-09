step({
    name = "hello",
    phase = "demo",
    scope = "default",
    run = "true",
})
workflow("run", {
    {},
    { phase = "demo", scope = "default" },
})
