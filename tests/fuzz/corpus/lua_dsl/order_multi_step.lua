order("alpha", "beta", "gamma")
step({ name = "gamma", phase = "p", scope = "sc", mutate = { "src/**" }, run = "true" })
step({ name = "beta", phase = "p", scope = "sc", mutate = { "src/**" }, run = "true" })
step({ name = "alpha", phase = "p", scope = "sc", mutate = { "src/**" }, run = "true" })
