local M = {}

function M.resolve_for_mode(step_config, mode)
    local cfg = step_config or {}
    if type(cfg.extra_args) ~= "table" then
        error("clang-format extra_args must be a table of strings")
    end

    return cfg
end

return M
