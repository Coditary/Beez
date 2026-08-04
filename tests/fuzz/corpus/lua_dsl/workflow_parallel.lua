workflow("ci", {
    { parallel = {
        { phase = "generate", scope = "docs" },
        { phase = "generate", scope = "code" },
    }},
    { phase = "compile", scope = "code" },
})
