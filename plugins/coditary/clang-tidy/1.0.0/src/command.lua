local M = {}

function M.quote(path)
    return "'" .. path:gsub("'", "'\\''") .. "'"
end

function M.build(config, path)
    local parts = {
        config.binary,
        "-p",
        M.quote(config.compdb),
        M.quote(path),
        "--header-filter=" .. M.quote(config.header_filter),
        "--use-color",
    }

    if config.checks ~= nil and config.checks ~= "" then
        parts[#parts + 1] = "--checks=" .. M.quote(config.checks)
    end

    for _, argument in ipairs(config.extra_args) do
        parts[#parts + 1] = argument
    end

    return table.concat(parts, " ")
end

return M
