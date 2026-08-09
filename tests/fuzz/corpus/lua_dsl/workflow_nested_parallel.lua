workflow("deep", {
    {
        parallel = {
            { phase = "a", scope = "1" },
            {
                parallel = {
                    { phase = "b", scope = "2" },
                    { phase = "c", scope = "3" },
                },
            },
            { phase = "d", scope = "4" },
        },
    },
    { phase = "e", scope = "5" },
})
step({ name = "s1", phase = "a", scope = "1", run = "true" })
step({ name = "s2", phase = "b", scope = "2", run = "true" })
step({ name = "s3", phase = "c", scope = "3", run = "true" })
step({ name = "s4", phase = "d", scope = "4", run = "true" })
step({ name = "s5", phase = "e", scope = "5", run = "true" })
