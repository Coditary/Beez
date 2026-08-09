step({
    name = "hello",
    phase = "demo",
    scope = "default",
    run = "echo minimal-valid > out.txt",
})

task("run", { { name = "hello" } })
workflow("default", { { phase = "demo", scope = "default" } })
