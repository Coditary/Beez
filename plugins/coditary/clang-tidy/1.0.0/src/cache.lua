local command = require("src.command")
local output = require("src.output")

local M = {}

function M.run(ctx, config)
    local runtime_config = beez.data.merge(config, { project_root = ctx.project_root })

    return beez.increment.run(ctx, {
        patterns = runtime_config.patterns,
        log_prefix = runtime_config.log_prefix,
        parallelism = runtime_config.parallelism,
        cache_key = runtime_config.cache_key,
        warnings_as_errors = runtime_config.warnings_as_errors,
        prereq = function()
            local compdb_file = runtime_config.compdb .. "/compile_commands.json"
            if not beez.fs.exists(compdb_file) then
                print(runtime_config.log_prefix .. " compile_commands.json not found under: " .. runtime_config.compdb)
                print(runtime_config.log_prefix .. " Run: make setup (or make debug) first.")
                return 2
            end
            return nil
        end,
        build_cmd = function(source_path)
            return command.build(runtime_config, source_path)
        end,
        output = {
            has_issues = function(tool_output, exit_code)
                return output.has_issues(runtime_config, tool_output, exit_code)
            end,
            filter_issues = function(tool_output)
                return output.filter_issues(runtime_config, tool_output)
            end,
            filter_failure = function(tool_output)
                return output.filter_failure(runtime_config, tool_output)
            end,
            describe_failure = function(source_path, exit_code, tool_output)
                return output.describe_failure(runtime_config, source_path, exit_code, tool_output)
            end,
            filter_verbose = output.filter_verbose,
        },
    })
end

return M
