-- Orphan task (manually triggerable)
task("clean", "rm -fr app.o")

-- Phase-bound tasks (stored, manually triggerable in milestone 1)
task("doxygen", { phase = "generate", scope = "docs", run = "echo 'doxygen...'" })
task("protobuf", { phase = "generate", scope = "code", run = "echo 'protoc...'" })
task("cpp:compile", { phase = "compile", scope = "code", run = "echo 'compiling...'" })
task("hi", "date")

-- Workflow (registered but not executable yet)
-- Steps run in order. A `parallel` block runs all listed phases at the same time.
workflow("build", {
    { phase = "generate", scope = "code" },
    { phase = "compile", scope = "code" },
})

workflow("ci", {
    { parallel = {
        { phase = "generate", scope = "docs" },
        { phase = "generate", scope = "code" },
    }},
    { phase = "compile", scope = "code" },
})
