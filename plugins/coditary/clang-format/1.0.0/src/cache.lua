local command = require("src.command")

local M = {}

function M.run(ctx, config, mode)
    return beez.increment.run(ctx, {
        patterns = config.patterns,
        log_prefix = config.log_prefix,
        spawn_all = true,
        action = mode == "check" and "checking" or "applying",
        build_cmd = function(source_path)
            return command.build(config, source_path, mode)
        end,
    })
end

return M
