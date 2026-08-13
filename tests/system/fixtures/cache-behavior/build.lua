step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/runs.txt" },
    run = "mkdir -p build && echo run >> build/runs.txt",
})

task("build", { { step = "compile" } })
