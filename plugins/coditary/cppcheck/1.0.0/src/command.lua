local M = {}

function M.build(config, path)
    local parts = {
        config.binary,
        "--enable=" .. config.enable,
        "--std=" .. config.std,
    }

    for _, include_path in ipairs(config.include_paths) do
        parts[#parts + 1] = "-I"
        parts[#parts + 1] = beez.char.quote(include_path)
    end

    for _, suppression in ipairs(config.suppressions) do
        parts[#parts + 1] = "--suppress=" .. suppression
    end

    if config.inline_suppr then
        parts[#parts + 1] = "--inline-suppr"
    end

    parts[#parts + 1] = "--error-exitcode=1"

    if config.quiet then
        parts[#parts + 1] = "--quiet"
    end

    for _, argument in ipairs(config.extra_args) do
        parts[#parts + 1] = argument
    end

    parts[#parts + 1] = beez.char.quote(path)

    return table.concat(parts, " ")
end

return M
