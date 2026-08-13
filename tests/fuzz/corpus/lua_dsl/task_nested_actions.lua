step({
    name = "a",
    phase = "build",
    scope = "code",
    run = "echo a",
})
step({
    name = "b",
    phase = "build",
    scope = "code",
    run = "echo b",
})
task("pipeline", {
    "echo start",
    { step = "a", config = { flags = "-O2" } },
    { parallel = { "echo p1", { step = "b" } } },
    { phase = "build", scope = "code", run = "echo inline" },
    "echo end",
})
