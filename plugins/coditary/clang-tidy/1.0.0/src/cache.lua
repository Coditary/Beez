local command = require("src.command")
local output = require("src.output")

local M = {}

function M.run(ctx, config)
    return beez.increment.run(ctx, {
        patterns = config.patterns,
        log_prefix = config.log_prefix,
        parallelism = config.parallelism,
        cache_key = config.cache_key,
        warnings_as_errors = config.warnings_as_errors,
        prereq = function()
            local compdb_file = config.compdb .. "/compile_commands.json"
            if not beez.fs.exists(compdb_file) then
                print(config.log_prefix .. " compile_commands.json not found under: " .. config.compdb)
                print(config.log_prefix .. " Run: make setup (or make debug) first.")
                return 2
            end
            return nil
        end,
        build_cmd = function(source_path)
            return command.build(config, source_path)
        end,
        output = {
            has_issues = function(tool_output, exit_code)
                return output.has_issues(config, tool_output, exit_code)
            end,
            filter_issues = function(tool_output)
                return output.filter_issues(config, tool_output)
            end,
            filter_failure = function(tool_output)
                return output.filter_failure(config, tool_output)
            end,
            filter_verbose = output.filter_verbose,
        },
    })
end

return M
