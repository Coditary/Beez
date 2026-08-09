beez.config({
    performance = {
        max_threads = 8,
        cache_write_strategy = "parallel",
        cache_fs_metadata = true,
        use_mmap_for_hashing = false,
        mmap_hashing_min_bytes = 65536,
        optimize_gc_for_throughput = true,
        pin_threads_to_cores = false,
    },
    cache = {
        path = "fuzz-cache",
        enabled = true,
        protect = true,
        hash = {
            algorithm = "fnv1a64",
            seed = 0,
        },
        compress = {
            algorithm = "zstd",
            level = 6,
            mode = "always",
        },
    },
    ui = {
        output_mode = "verbose",
        colors = true,
        truecolor = false,
        theme = "default",
        icons = true,
        animation = {
            progress = "minimal",
            indicator = "percent",
        },
        hide_cache_hits = false,
        prefix = true,
        prefix_format = "Worker {name}",
        show_time_saved = true,
        summary = "full",
        logging = {
            run_log = true,
            log_steps = true,
            workers = "failures",
        },
    },
    env = {
        load_dotenv = true,
        dotenv_overrides_system = false,
        vars = {
            BUILD_TYPE = "Release",
            CC = "clang",
        },
        hash_vars = "PATH,HOME",
        ignore_vars_for_hashing = "TMPDIR",
        mask_secrets = "TOKEN,SECRET",
    },
})
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "true",
})
workflow("build", {
    { phase = "generate", scope = "code" },
})
