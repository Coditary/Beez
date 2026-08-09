beez.config({
    cache = {
        hash = { algorithm = "crc32", seed = 4294967295 },
        compress = { algorithm = "gzip", level = 22, mode = "never" },
    },
    performance = {
        max_threads = 999999,
        mmap_hashing_min_bytes = -1,
        cache_write_strategy = "not-a-strategy",
    },
    ui = {
        output_mode = "not-a-mode",
        animation = { progress = "???", indicator = "???" },
        logging = { workers = "all" },
    },
})
task("edge", "true")
