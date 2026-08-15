step({
    name = "cpp:compile",
    phase = "compile",
    scope = "code",
    run = "echo compile",
})
task("full_build", {
    "echo start",
    { step = "cpp:compile", config = { optimize = "-O3" } },
    "echo done",
})
