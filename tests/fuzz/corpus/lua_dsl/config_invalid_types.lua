beez.config({
    cache = {
        path = 42,
        enabled = "yes",
        hash = {
            algorithm = "unknown-algo",
            seed = -1,
        },
        compress = {
            level = "high",
            mode = "sometimes",
        },
    },
    performance = {
        max_threads = "many",
    },
    ui = {
        output_mode = "loud",
        animation = {
            progress = 123,
        },
    },
    env = {
        vars = "not-a-table",
    },
})
task("noop", "true")
