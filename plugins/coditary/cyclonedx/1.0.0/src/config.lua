local defaults = require("src.defaults")
local shell = require("src.shell")

local M = {}

local function copy_string_array(values, fallback)
    if values == nil then
        return fallback
    end

    if type(values) ~= "table" then
        error("cyclonedx array config fields must be tables of strings")
    end

    local copied = {}
    for _, entry in ipairs(values) do
        if type(entry) ~= "string" then
            error("cyclonedx array config entries must be strings")
        end
        copied[#copied + 1] = entry
    end

    return copied
end

function M.resolve(config)
    local cfg = config or {}

    return {
        python_binary = cfg.python_binary or defaults.python_binary,
        cyclonedx_cli = cfg.cyclonedx_cli or defaults.cyclonedx_cli,
        sbom_dir = cfg.sbom_dir or defaults.sbom_dir,
        cyclonedx_json = cfg.cyclonedx_json or defaults.cyclonedx_json,
        merged_json = cfg.merged_json or defaults.merged_json,
        check_script = cfg.check_script or defaults.check_script,
        merge_script = cfg.merge_script or defaults.merge_script,
        merge_inputs = copy_string_array(cfg.merge_inputs, defaults.default_merge_inputs),
        log_prefix_check = cfg.log_prefix_check or defaults.log_prefix_check,
        log_prefix_merge = cfg.log_prefix_merge or defaults.log_prefix_merge,
        check_rev = cfg.check_rev or defaults.check_rev,
        merge_rev = cfg.merge_rev or defaults.merge_rev,
        use_cli_merge = cfg.use_cli_merge == true,
    }
end

function M.abs_path(root, relative)
    return root .. "/" .. relative
end

return M
