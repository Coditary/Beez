local M = {}

function M.build(config, path)
    local parts = {
        config.binary,
        "-p",
        beez.char.quote(config.compdb),
        beez.char.quote(path),
        "--header-filter=" .. beez.char.quote(config.header_filter),
        "--use-color",
    }

    if config.checks ~= nil and config.checks ~= "" then
        parts[#parts + 1] = "--checks=" .. beez.char.quote(config.checks)
    end

    for _, argument in ipairs(config.extra_args) do
        parts[#parts + 1] = argument
    end

    return table.concat(parts, " ")
end

return M
