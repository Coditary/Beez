local shell = require("src.shell")

local M = {}

local function env_exports(config)
    local parts = {
        "REPORTS_DIR=" .. shell.quote(config.reports_dir),
    }

    if config.fuzzer_time ~= nil and config.fuzzer_time ~= "" then
        parts[#parts + 1] = "FUZZER_TIME=" .. shell.quote(config.fuzzer_time)
    end

    if config.fuzzer_profile ~= nil and config.fuzzer_profile ~= "" then
        parts[#parts + 1] = "FUZZER_PROFILE=" .. shell.quote(config.fuzzer_profile)
    end

    if config.fuzzer_rss_limit_mb ~= nil and config.fuzzer_rss_limit_mb ~= "" then
        parts[#parts + 1] = "FUZZER_RSS_LIMIT_MB=" .. shell.quote(config.fuzzer_rss_limit_mb)
    end

    return table.concat(parts, " ")
end

function M.script(config, root)
    local script_path = root .. "/" .. config.script
    local parts = {
        env_exports(config),
        shell.quote(script_path),
        shell.quote(config.build_dir),
    }

    return table.concat(parts, " ")
end

function M.direct(config, root)
    local script_path = root .. "/" .. config.script
    return shell.quote(script_path)
end

return M
