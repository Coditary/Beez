beez.config({
    cache = {
        hash = {
            algorithm = "crc32",
            seed = 7,
        },
        compress = {
            algorithm = "gzip",
            level = 3,
            mode = "default",
        },
        path = "nested-cache",
        enabled = true,
        protect = false,
    },
    performance = {
        max_threads = 4,
        cache_write_strategy = "parallel",
    },
    ui = {
        output_mode = "clean",
        animation = {
            progress = "blocks",
            indicator = "spinner",
        },
    },
    env = {
        vars = {
            NESTED = "value",
        },
    },
})
task("noop", "true")
