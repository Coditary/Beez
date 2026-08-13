local defaults = require("src.defaults")
local step_config = require("src.step_config")

-- Beez CycloneDX plugin
--
-- Steps:
--   cyclonedx_check  — validate CycloneDX BOM structure
--   cyclonedx_merge  — merge multiple BOM files into cyclonedx-merged.json
--
-- configure_step("cyclonedx_merge", {
--     merge_inputs = { "report/sbom/cyclonedx.json", "extra/sbom.json" },
--     merged_json = "report/sbom/cyclonedx-merged.json",
--     merge_rev = "1",
-- })
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/cyclonedx",
--             path = "./plugins/coditary/cyclonedx",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("cyclonedx", {
    version = "1.0.0",
    description = "CycloneDX SBOM validation and merge",
    organization = "coditary",

    steps = {
        cyclonedx_check = {
            phase = "qa",
            scope = "supply",
            input = { defaults.cyclonedx_json },
            output = { defaults.cyclonedx_json },
            description = "Validate CycloneDX SBOM",
            config = step_config.check_defaults(),
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        cyclonedx_merge = {
            phase = "qa",
            scope = "supply",
            input = defaults.default_merge_inputs,
            output = { defaults.merged_json },
            description = "Merge CycloneDX SBOM files",
            config = step_config.merge_defaults(),
            run = function(ctx)
                return require("src.runner").merge(ctx)
            end,
        },
    },
})
