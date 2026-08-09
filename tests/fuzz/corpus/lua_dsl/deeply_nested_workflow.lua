workflow("deep", {
    { parallel = {
        { parallel = {
            { phase = "generate", scope = "docs" },
            { phase = "generate", scope = "code" },
        }},
        { phase = "compile", scope = "code" },
    }},
    { parallel = {
        { phase = "test", scope = "unit" },
        { phase = "test", scope = "integration" },
    }},
    { phase = "package", scope = "release" },
})
