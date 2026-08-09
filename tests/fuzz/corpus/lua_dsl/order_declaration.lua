order("cpp:lint", "cpp:format")
step({
    name = "cpp:format",
    phase = "compile",
    scope = "cpp",
    mutate = { "src/**/*.cpp" },
    run = "echo format",
})
step({
    name = "cpp:lint",
    phase = "compile",
    scope = "cpp",
    mutate = { "src/**/*.cpp" },
    run = "echo lint",
})
