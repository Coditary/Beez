step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/**/*.o" },
    mutate = { "src/**/*.cpp" },
    description = "Compile sources",
    run = "echo compile",
})
