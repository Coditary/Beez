task("hello", "echo hello > hello.out")

task("gen-code", { phase = "generate", scope = "code", run = "echo code > code.out" })

workflow("build", {
    { phase = "generate", scope = "code" },
    { phase = "compile", scope = "code" },
})
