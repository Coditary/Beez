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

local function merge_profile(cfg, profile_name, use_profile_cache_key)
    local profile = defaults.profiles[profile_name]
    if profile == nil then
        error("unknown cppcheck profile: " .. profile_name)
    end

    local run_cfg = beez.data.merge(cfg, profile)
    if use_profile_cache_key then
        run_cfg.cache_key = profile_name
    end

    return run_cfg
end

function M.expand_runs(cfg)
    local profile_names = cfg.profiles
    if profile_names == nil then
        profile_names = { "analyze" }
    end

    if type(profile_names) ~= "table" or #profile_names == 0 then
        error("cppcheck profiles must be a non-empty array (analyze, security)")
    end

    local use_profile_cache_key = #profile_names > 1
    local runs = {}

    for _, profile_name in ipairs(profile_names) do
        if type(profile_name) ~= "string" then
            error("cppcheck profile names must be strings")
        end

        runs[#runs + 1] = merge_profile(cfg, profile_name, use_profile_cache_key)
    end

    return runs
end

return M
