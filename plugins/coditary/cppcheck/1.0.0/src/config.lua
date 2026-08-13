local defaults = require("src.defaults")

local M = {}

function M.normalize_enable(enable)
    if enable == nil then
        return defaults.enable
    end

    if type(enable) == "string" then
        if enable == "" then
            return defaults.enable
        end
        return enable
    end

    if type(enable) ~= "table" or #enable == 0 then
        error("cppcheck enable must be a string or non-empty array of check categories")
    end

    for _, entry in ipairs(enable) do
        if type(entry) ~= "string" then
            error("cppcheck enable array entries must be strings")
        end
    end

    return table.concat(enable, ",")
end

local function copy_string_array(values, fallback)
    if values == nil then
        return fallback
    end

    if type(values) ~= "table" then
        error("cppcheck array config fields must be tables of strings")
    end

    local copied = {}
    for _, entry in ipairs(values) do
        if type(entry) ~= "string" then
            error("cppcheck array config entries must be strings")
        end
        copied[#copied + 1] = entry
    end

    return copied
end

local function merge_profile(step_config, profile_name, profile, use_profile_cache_key)
    local merged = {
        patterns = step_config.patterns or profile.patterns,
        binary = step_config.binary,
        std = step_config.std,
        enable = step_config.enable,
        include_paths = step_config.include_paths,
        suppressions = step_config.suppressions,
        inline_suppr = step_config.inline_suppr,
        quiet = step_config.quiet,
        issue_path_pattern = step_config.issue_path_pattern,
        exclude_substrings = step_config.exclude_substrings,
        parallelism = step_config.parallelism,
        extra_args = step_config.extra_args,
        warnings_as_errors = step_config.warnings_as_errors,
        log_prefix = profile.log_prefix,
        worker_prefix = profile.worker_prefix,
    }

    if use_profile_cache_key then
        merged.cache_key = profile_name
    end

    return M.resolve(merged)
end

function M.resolve(step_config)
    local config = step_config or {}

    local extra_args = {}
    if config.extra_args ~= nil then
        for _, argument in ipairs(config.extra_args) do
            extra_args[#extra_args + 1] = argument
        end
    end

    local resolved = {
        patterns = config.patterns or defaults.patterns_analyze,
        binary = config.binary or defaults.binary,
        std = config.std or defaults.std,
        enable = M.normalize_enable(config.enable),
        include_paths = copy_string_array(config.include_paths, defaults.include_paths),
        suppressions = copy_string_array(config.suppressions, defaults.suppressions),
        inline_suppr = config.inline_suppr ~= false,
        quiet = config.quiet ~= false,
        issue_path_pattern = config.issue_path_pattern or defaults.issue_path_pattern,
        exclude_substrings = copy_string_array(config.exclude_substrings, defaults.exclude_substrings),
        parallelism = config.parallelism or defaults.parallelism,
        extra_args = extra_args,
        warnings_as_errors = config.warnings_as_errors or defaults.warnings_as_errors,
        log_prefix = config.log_prefix or defaults.log_prefix_check,
        worker_prefix = config.worker_prefix or defaults.worker_prefix_check,
        cache_key = config.cache_key,
    }

    return resolved
end

function M.expand_runs(step_config)
    local config = step_config or {}

    local profiles = config.profiles
    if profiles == nil then
        profiles = { "analyze" }
    end

    if type(profiles) ~= "table" or #profiles == 0 then
        error("cppcheck profiles must be a non-empty array (analyze, security)")
    end

    local use_profile_cache_key = #profiles > 1
    local runs = {}

    for _, profile_name in ipairs(profiles) do
        if type(profile_name) ~= "string" then
            error("cppcheck profile names must be strings")
        end

        local profile = defaults.profiles[profile_name]
        if profile == nil then
            error("unknown cppcheck profile: " .. profile_name ..
                " (expected analyze or security)")
        end

        runs[#runs + 1] = merge_profile(config, profile_name, profile, use_profile_cache_key)
    end

    return runs
end

return M
