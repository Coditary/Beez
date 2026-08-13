beez.config({
    cache = {
        path = "fixture-cache",
        protect = true,
    },
    performance = {
        max_threads = 4,
    },
})

step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/main.o" },
    run = "mkdir -p build && echo object > build/main.o",
})

task("build", { { step = "compile" } })
task("noop", "true")
