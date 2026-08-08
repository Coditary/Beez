return {
    performance = {
        max_threads = 8,
        cache_write_strategy = "phase",
        cache_fs_metadata = true,
        use_mmap_for_hashing = true,
        mmap_hashing_min_bytes = 65536,
        optimize_gc_for_throughput = false,
        pin_threads_to_cores = false,
    },

    cache = {
        path = ".cache",
        enabled = true,
        protect = true,
        hash = {
            algorithm = "fnv1a64",
            seed = 0,
        },
        compress = {
            algorithm = "gzip",
            level = 6,
            mode = "always",
        },
    },

    ui = {
        output_mode = "clean",
	    --    colors = true,
	    --    truecolor = true,
	    --    themes = {
	    --        gruvbox = {
	    --            text = "#ebdbb2",
	    --            muted = "#928374",
	    --            success = "#b8bb26",
	    --            warning = "#fabd2f",
	    --            error = "#fb4934",
	    --            info = "#83a598",
	    --            accent = "#fe8019",
	    --            progress_fill = "#b8bb26",
	    --            progress_empty = "#665c54",
	    --            cache_hit = "#8ec07c",
	    --            worker_prefix = "#83a598",
	    --        },
	    --    },
	    --    theme = "gruvbox",
	    --    icons = true,
	    --    animation = {
	    --        progress = "minimal",
	    --        indicator = "dots",
	    -- indicator_spin_interval = 80,
	    --    },
	    --    log_level = "info",
	    --    hide_cache_hits = false,
	    --    prefix = true,
	    --    prefix_format = "[Worker {id}",
	    --    show_time_saved = true,
    },

    env = {
        load_dotenv = true,
        dotenv_overrides_system = false,
        files = ".env",
        vars = {
            BUILD_TYPE = "Release",
            CONAN_PROFILE = "conan/profiles/clang-release",
            REPORTS_DIR = "report",
            FUZZER_TIME = "30",
            CC = "clang",
            CXX = "clang++",
        },
        hash_vars = {
            "CC",
            "CXX",
            "CFLAGS",
            "CXXFLAGS",
            "LDFLAGS",
            "BUILD_TYPE",
        },
        ignore_vars_for_hashing = {
            "TERM",
            "COLORTERM",
            "PWD",
            "SESSION_ID",
        },
        mask_secrets = {
            "AWS_ACCESS_KEY_ID",
            "AWS_SECRET_ACCESS_KEY",
            "GITHUB_TOKEN",
            "NPM_AUTH_TOKEN",
        },
    },
}
