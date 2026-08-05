configure_step("shader", {
    shader_version = "450",
    optimize = true,
})
step({
    name = "shader",
    phase = "generate",
    scope = "code",
    config = {
        output_dir = "build/shaders",
    },
    run = "echo shader",
})
