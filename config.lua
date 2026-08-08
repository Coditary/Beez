return {
    performance = {
        max_threads = 8,
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
    },

    paths = {
        env_file = ".env",
        build_script = "build.lua",
    },

    engine = {
        dry_run = false,
        enable_cache = true,
    },

    environment = {
        BUILD_TYPE = "Release",
        CONAN_PROFILE = "conan/profiles/clang-release",
        REPORTS_DIR = "report",
        FUZZER_TIME = "30",
        CC = "clang",
        CXX = "clang++",
    },
}
