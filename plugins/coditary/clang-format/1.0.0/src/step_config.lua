local defaults = require("src.defaults")

local M = {}

function M.defaults()
    return {
        patterns = defaults.patterns,
        binary = defaults.binary,
        format_rev = defaults.format_rev,
        werror = defaults.werror,
    }
end

return M
