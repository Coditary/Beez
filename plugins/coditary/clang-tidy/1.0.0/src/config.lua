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

function M.default_compdb()
    local build_type = beez.env("BUILD_TYPE") or "Release"
    return "build/build/" .. build_type
end

local function merge_profile(cfg, profile_name, profile, use_profile_cache_key)
    local run_cfg = beez.data.merge(cfg, profile)
    if use_profile_cache_key then
        run_cfg.cache_key = profile_name
    end
    return run_cfg
end

function M.expand_runs(cfg)
    if cfg.checks ~= nil then
        return { cfg }
    end

    local profile_names = cfg.profiles or { "lint" }
    if type(profile_names) ~= "table" or #profile_names == 0 then
        error("clang-tidy profiles must be a non-empty array (lint, analyze, security)")
    end

    local use_profile_cache_key = #profile_names > 1
    local runs = {}

    for _, profile_name in ipairs(profile_names) do
        if type(profile_name) ~= "string" then
            error("clang-tidy profile names must be strings")
        end

        local profile = defaults.profiles[profile_name]
        if profile == nil then
            error("unknown clang-tidy profile: " .. profile_name ..
                " (expected lint, analyze, or security)")
        end

        runs[#runs + 1] = merge_profile(cfg, profile_name, profile, use_profile_cache_key)
    end

    return runs
end

return M
