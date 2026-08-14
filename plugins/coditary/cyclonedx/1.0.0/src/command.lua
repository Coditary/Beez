
local M = {}

function M.check(config, root)
    local bom_path = root .. "/" .. config.cyclonedx_json
    local script = root .. "/" .. config.check_script

    local parts = {
        config.python_binary,
        beez.char.quote(script),
        beez.char.quote(bom_path),
    }

    return table.concat(parts, " ")
end

function M.merge_python(config, root)
    local output = root .. "/" .. config.merged_json
    local script = root .. "/" .. config.merge_script

    local parts = {
        config.python_binary,
        beez.char.quote(script),
        beez.char.quote(output),
    }

    for _, input in ipairs(config.merge_inputs) do
        parts[#parts + 1] = beez.char.quote(root .. "/" .. input)
    end

    return table.concat(parts, " ")
end

function M.merge_cli(config, root)
    local output = root .. "/" .. config.merged_json

    local parts = {
        config.cyclonedx_cli,
        "merge",
        "--output-file",
        beez.char.quote(output),
        "--output-format",
        "json",
    }

    for _, input in ipairs(config.merge_inputs) do
        parts[#parts + 1] = "--input-files"
        parts[#parts + 1] = beez.char.quote(root .. "/" .. input)
    end

    return table.concat(parts, " ")
end

function M.mkdir_sbom_dir(config, root)
    return "mkdir -p " .. beez.char.quote(root .. "/" .. config.sbom_dir)
end

return M
