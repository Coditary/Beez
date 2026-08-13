local defaults = require("src.defaults")

local M = {}

function M.compile_defaults(profile_name)
    return {
        profile = profile_name,
        compile_rev = defaults.compile_rev,
        log_prefix = defaults.log_prefix_compile,
        cache_key = profile_name .. ":compile",
    }
end

function M.link_defaults(profile_name)
    return {
        profile = profile_name,
        link_rev = defaults.link_rev,
        log_prefix = defaults.log_prefix_link,
        cache_key = profile_name .. ":link",
    }
end

return M
