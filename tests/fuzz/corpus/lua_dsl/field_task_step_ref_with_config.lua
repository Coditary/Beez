step({ name = "s", phase = "p", scope = "sc", run = "true" })
task("run", { { name = "s", config = { flag = true } } })
