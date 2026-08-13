local defaults = require("src.defaults")

local M = {}

function M.resolve(step_config)
    local config = step_config or {}

    local resolved = {
        patterns = config.patterns or defaults.patterns,
        binary = config.binary or defaults.binary,
        style = config.style,
        fallback_style = config.fallback_style,
        extra_args = config.extra_args or {},
        werror = config.werror ~= false,
        log_prefix = config.log_prefix or defaults.log_prefix_check,
        worker_prefix = config.worker_prefix or defaults.worker_prefix_check,
    }

    if type(resolved.extra_args) ~= "table" then
        error("clang-format extra_args must be a table of strings")
    end

  return resolved
end

function M.resolve_for_mode(step_config, mode)
    local resolved = M.resolve(step_config)
    if mode == "apply" then
        resolved.log_prefix = step_config and step_config.log_prefix or defaults.log_prefix_apply
        resolved.worker_prefix = step_config and step_config.worker_prefix or defaults.worker_prefix_apply
    end
    return resolved
end

return M
