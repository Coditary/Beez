step({
    name = "a",
    phase = "build",
    scope = "code",
    run = "true",
})
step({
    name = "b",
    phase = "build",
    scope = "code",
    run = "true",
})
step({
    name = "c",
    phase = "test",
    scope = "code",
    run = "true",
})
order("a", "b")
order("b", "c")
order("c", "a")
workflow("cycle", {
    { phase = "build", scope = "code" },
    { phase = "test", scope = "code" },
})
