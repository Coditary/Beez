local defaults = require("src.defaults")

plugin("cyclonedx", {
    version = "1.0.0",
    description = "CycloneDX SBOM validation and merge",
    organization = "coditary",

    config = {
        defaults = {
            python_binary = defaults.python_binary,
            cyclonedx_cli = defaults.cyclonedx_cli,
            sbom_dir = defaults.sbom_dir,
            cyclonedx_json = defaults.cyclonedx_json,
            merged_json = defaults.merged_json,
            check_script = defaults.check_script,
            merge_script = defaults.merge_script,
            merge_inputs = defaults.default_merge_inputs,
            log_prefix_check = defaults.log_prefix_check,
            log_prefix_merge = defaults.log_prefix_merge,
            check_rev = defaults.check_rev,
            merge_rev = defaults.merge_rev,
        },
    },

    steps = {
        cyclonedx_check = {
            phase = "package",
            scope = "audit",
            input = { defaults.cyclonedx_json },
            output = { defaults.cyclonedx_json },
            description = "Validate CycloneDX SBOM",
            config = {},
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        cyclonedx_merge = {
            phase = "package",
            scope = "audit",
            input = defaults.default_merge_inputs,
            output = { defaults.merged_json },
            description = "Merge CycloneDX SBOM files",
            config = {},
            run = function(ctx)
                return require("src.runner").merge(ctx)
            end,
        },
    },
})
