local M = {}

function M.env_or(key, default)
    local value = beez.env(key)
    if value == nil then
        return default
    end
    return value
end

return M
