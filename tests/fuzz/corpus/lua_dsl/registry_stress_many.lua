for i = 1, 32 do
    step({
        name = "step-" .. i,
        phase = "phase-" .. (i % 4),
        scope = "scope-" .. (i % 3),
        run = "echo " .. i,
    })
end

for i = 1, 16 do
    task("task-" .. i, {
        { name = "step-" .. i },
        "echo task-" .. i,
    })
end

workflow("stress", {
    { phase = "phase-0", scope = "scope-0" },
    { phase = "phase-1", scope = "scope-1" },
    { phase = "phase-2", scope = "scope-2" },
    { phase = "phase-3", scope = "scope-0" },
})
