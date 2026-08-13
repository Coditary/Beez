local defaults = require("src.defaults")

local M = {}

function M.run_defaults(run_name)
    return {
        run = run_name,
        fuzz_rev = defaults.fuzz_rev,
        log_prefix = defaults.runs[run_name].log_prefix,
    }
end

return M
