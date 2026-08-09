beez.config({
    performance = { max_threads = 1 },
    cache = {
        path = "enum-cache",
        enabled = true,
        protect = false,
        hash = { algorithm = "fnv1a64", seed = 0 },
        compress = { algorithm = "zstd", level = 1, mode = "always" },
    },
    ui = {
        output_mode = "verbose",
        colors = false,
        truecolor = false,
        theme = "default",
        icons = true,
        animation = { progress = "minimal", indicator = "percent" },
        hide_cache_hits = true,
        prefix = false,
        prefix_format = "{name}",
        show_time_saved = false,
        summary = "compact",
        logging = { run_log = false, log_steps = false, workers = "off" },
    },
    env = {
        load_dotenv = true,
        dotenv_overrides_system = false,
        vars = { CC = "clang", CXX = "clang++" },
        hash_vars = "PATH",
        ignore_vars_for_hashing = "TMPDIR",
        mask_secrets = "TOKEN",
    },
})
step({ name = "s", phase = "p", scope = "sc", run = "true" })
workflow("w", { { phase = "p", scope = "sc" } })
