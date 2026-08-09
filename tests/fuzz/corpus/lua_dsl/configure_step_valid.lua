step({
    name = "run",
    phase = "demo",
    scope = "default",
    run = "true",
})
configure_step("run", {
    performance = { max_threads = 4 },
    cache = { enabled = true, protect = false },
})
task("build", { { name = "run", config = { optimize = "-O3" } } })
workflow("default", { { phase = "demo", scope = "default" } })
