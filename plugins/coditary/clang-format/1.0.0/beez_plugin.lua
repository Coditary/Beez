local defaults = require("src.defaults")

plugin("clang-format", {
    version = "1.0.0",
    description = "Incremental clang-format check and apply",
    organization = "coditary",

    config = {
        defaults = {
            patterns = defaults.patterns,
            binary = defaults.binary,
            extra_args = {},
            werror = defaults.werror,
        },

        profile_defs = {
            check = {
                format_rev = defaults.format_rev,
                log_prefix = defaults.log_prefix_check,
                worker_prefix = defaults.worker_prefix_check,
            },
            apply = {
                format_rev = defaults.format_rev,
                log_prefix = defaults.log_prefix_apply,
                worker_prefix = defaults.worker_prefix_apply,
            },
        },
    },

    steps = {
        qa_check = {
            phase = "quality",
            scope = "format",
            input = defaults.patterns,
            description = "clang-format check (incremental)",
            config = { profile = "check" },
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        format_apply = {
            phase = "quality",
            scope = "format",
            mutate = defaults.patterns,
            description = "Apply clang-format (incremental)",
            config = { profile = "apply" },
            run = function(ctx)
                return require("src.runner").apply(ctx)
            end,
        },
    },
})
