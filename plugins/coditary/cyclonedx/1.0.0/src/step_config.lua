local defaults = require("src.defaults")

local M = {}

function M.check_defaults()
    return {
        check_rev = defaults.check_rev,
        cyclonedx_json = defaults.cyclonedx_json,
        log_prefix_check = defaults.log_prefix_check,
    }
end

function M.merge_defaults()
    return {
        merge_rev = defaults.merge_rev,
        merge_inputs = defaults.default_merge_inputs,
        merged_json = defaults.merged_json,
        sbom_dir = defaults.sbom_dir,
        log_prefix_merge = defaults.log_prefix_merge,
    }
end

return M
