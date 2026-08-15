step({
    name = "hello",
    phase = "demo",
    scope = "default",
    run = "echo minimal-valid > out.txt",
})

task("run", { { step = "hello" } })
workflow("default", { { phase = "demo", scope = "default" } })
