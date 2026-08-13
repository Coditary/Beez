local defaults = require("src.defaults")

local M = {}

function M.suite_defaults(suite_name)
    return {
        suite = suite_name,
        test_rev = defaults.test_rev,
        log_prefix = defaults.suites[suite_name].log_prefix,
    }
end

return M
