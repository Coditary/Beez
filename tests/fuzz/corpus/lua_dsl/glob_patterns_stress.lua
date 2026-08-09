step({
    name = "globby",
    phase = "build",
    scope = "code",
    input = {
        "src/**/*.cpp",
        "src/**/{a,b,c}.h",
        "build/*.o",
        "**/*.{c,h,hpp}",
    },
    output = {
        "build/**",
        "out/*.txt",
    },
    mutate = {
        "cache/**",
    },
    run = "true",
})
