step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/**/*.o" },
    run = function(ctx)
        local jobs = {}
        for _, source_file in ipairs({ "main.cpp", "util.cpp" }) do
            local obj_file = source_file:gsub("%.cpp$", ".o")
            local job = ctx:spawn({
                name = "compile_" .. source_file,
                cmd = "echo compile " .. source_file,
                inputs = { source_file },
                outputs = { "build/" .. obj_file },
            })
            table.insert(jobs, job)
        end
        return ctx:wait_all(jobs)
    end,
})
