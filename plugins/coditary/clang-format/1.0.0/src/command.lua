local M = {}

function M.quote(path)
    return "'" .. path:gsub("'", "'\\''") .. "'"
end

function M.build(config, path, mode)
    local parts = { config.binary }

    if config.style ~= nil and config.style ~= "" then
        parts[#parts + 1] = "-style=" .. config.style
    end

    if config.fallback_style ~= nil and config.fallback_style ~= "" then
        parts[#parts + 1] = "-fallback-style=" .. config.fallback_style
    end

    for _, argument in ipairs(config.extra_args) do
        parts[#parts + 1] = argument
    end

    if mode == "check" then
        parts[#parts + 1] = "--dry-run"
        if config.werror then
            parts[#parts + 1] = "--Werror"
        end
    else
        parts[#parts + 1] = "-i"
    end

    parts[#parts + 1] = M.quote(path)
    return table.concat(parts, " ")
end

return M
