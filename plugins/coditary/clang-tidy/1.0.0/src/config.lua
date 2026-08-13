local defaults = require("src.defaults")

local M = {}

function M.normalize_checks(checks)
    if checks == nil then
        return nil
    end

    if type(checks) == "string" then
        if checks == "" then
            return nil
        end
        return checks
    end

    if type(checks) ~= "table" then
        error("clang-tidy checks must be a string or array of check names/patterns")
    end

    if #checks == 0 then
        return nil
    end

    local has_disable_all = false
    for _, entry in ipairs(checks) do
        if type(entry) ~= "string" then
            error("clang-tidy checks array entries must be strings")
        end
        if entry == "-*" then
            has_disable_all = true
            break
        end
    end

    if not has_disable_all then
        return "-*," .. table.concat(checks, ",")
    end

    return table.concat(checks, ",")
end

local function merge_profile(step_config, profile_name, profile, use_profile_cache_key)
    local merged = {
        compdb = step_config.compdb or M.default_compdb(),
        patterns = step_config.patterns or profile.patterns,
        binary = step_config.binary,
        header_filter = step_config.header_filter,
        issue_path_pattern = step_config.issue_path_pattern,
        exclude_substrings = step_config.exclude_substrings,
        checks = profile.checks,
        extra_args = step_config.extra_args,
        log_prefix = profile.log_prefix,
        worker_prefix = profile.worker_prefix,
    }

    if use_profile_cache_key then
        merged.cache_key = profile_name
    end

    return M.resolve(merged)
end

function M.default_compdb()
    local build_type = beez.env("BUILD_TYPE") or "Release"
    return "build/build/" .. build_type
end

function M.resolve(step_config)
    local config = step_config or {}

    local compdb = config.compdb
    if compdb == nil or compdb == "" then
        compdb = M.default_compdb()
    end

    local extra_args = {}
    if config.extra_args ~= nil then
        for _, argument in ipairs(config.extra_args) do
            extra_args[#extra_args + 1] = argument
        end
    end

    if config.warnings_as_errors then
        extra_args[#extra_args + 1] = "--warnings-as-errors=*"
    end

    local resolved = {
        compdb = compdb,
        patterns = config.patterns or defaults.patterns_all,
        binary = config.binary or defaults.binary,
        header_filter = config.header_filter or defaults.header_filter,
        issue_path_pattern = config.issue_path_pattern or defaults.issue_path_pattern,
        exclude_substrings = config.exclude_substrings or defaults.exclude_substrings,
        checks = M.normalize_checks(config.checks),
        extra_args = extra_args,
        parallelism = config.parallelism or defaults.parallelism,
        warnings_as_errors = config.warnings_as_errors or defaults.warnings_as_errors,
        log_prefix = config.log_prefix or defaults.log_prefix_check,
        worker_prefix = config.worker_prefix or defaults.worker_prefix_check,
        cache_key = config.cache_key,
    }

    if type(resolved.extra_args) ~= "table" then
        error("clang-tidy extra_args must be a table of strings")
    end

    if type(resolved.exclude_substrings) ~= "table" then
        error("clang-tidy exclude_substrings must be a table of strings")
    end

    return resolved
end

function M.expand_runs(step_config)
    local config = step_config or {}

    if config.checks ~= nil then
        return { M.resolve(config) }
    end

    local profiles = config.profiles
    if profiles == nil then
        profiles = { "lint" }
    end

    if type(profiles) ~= "table" or #profiles == 0 then
        error("clang-tidy profiles must be a non-empty array (lint, analyze, security)")
    end

    local use_profile_cache_key = #profiles > 1
    local runs = {}

    for _, profile_name in ipairs(profiles) do
        if type(profile_name) ~= "string" then
            error("clang-tidy profile names must be strings")
        end

        local profile = defaults.profiles[profile_name]
        if profile == nil then
            error("unknown clang-tidy profile: " .. profile_name ..
                " (expected lint, analyze, or security)")
        end

        runs[#runs + 1] = merge_profile(config, profile_name, profile, use_profile_cache_key)
    end

    return runs
end

return M
