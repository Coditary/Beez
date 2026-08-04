task("gen-docs", { phase = "generate", scope = "docs", run = "echo docs > docs.out" })
task("gen-code", { phase = "generate", scope = "code", run = "echo code > code.out" })
task("compile", { phase = "compile", scope = "code", run = "echo built > build.out" })
