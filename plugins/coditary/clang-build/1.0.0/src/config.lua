local defaults = require("src.defaults")

local M = {}

function M.resolve_build_tree(profile, step_cfg)
    if step_cfg.build_tree ~= nil then
        return step_cfg.build_tree
    end

    if profile.build_tree ~= nil then
        return profile.build_tree
    end

    if profile.build_tree_from_build_type then
        local build_type = beez.env_or("BUILD_TYPE", "Release")
        return "build/build/" .. build_type
    end

    return defaults.debug_build_tree
end

function M.resolve(step_cfg, profile_name, root)
    local cfg = step_cfg or {}
    local profile = defaults.build_profiles[profile_name]
    if profile == nil then
        error("unknown clang-build profile: " .. tostring(profile_name))
    end

    local build_tree = M.resolve_build_tree(profile, cfg)

    return {
        profile_name = profile_name,
        scope = profile.scope,
        build_tree = build_tree,
        build_tree_abs = root .. "/" .. build_tree,
        index_lua = build_tree .. "/.beez-clang-index.lua",
        index_script = cfg.index_script or defaults.index_script,
        python_binary = cfg.python_binary or defaults.python_binary,
        cxx = cfg.cxx or beez.env_or("CXX", "clang++"),
        cc = cfg.cc or beez.env_or("CC", "clang"),
        log_prefix_compile = cfg.log_prefix_compile or cfg.log_prefix or defaults.log_prefix_compile,
        log_prefix_link = cfg.log_prefix_link or defaults.log_prefix_link,
        compile_rev = cfg.compile_rev or defaults.compile_rev,
        link_rev = cfg.link_rev or defaults.link_rev,
        parallelism = cfg.parallelism or 16,
        cache_key = cfg.cache_key or profile_name,
    }
end

return M
