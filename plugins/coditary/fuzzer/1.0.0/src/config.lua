local defaults = require("src.defaults")

local M = {}

function M.resolve(step_cfg, run_name)
    local cfg = step_cfg or {}
    local run = defaults.runs[run_name]
    if run == nil then
        error("unknown fuzzer run: " .. tostring(run_name))
    end

    local fuzzer_time = cfg.fuzzer_time or run.fuzzer_time
    if fuzzer_time == nil and run.fuzzer_time_env ~= nil then
        fuzzer_time = beez.env_or(run.fuzzer_time_env, run.fuzzer_time_default)
    end

    return {
        run_name = run_name,
        scope = run.scope,
        script = cfg.script or run.script,
        report_file = cfg.report_file or run.report_file,
        reports_dir = cfg.reports_dir or beez.env_or("REPORTS_DIR", defaults.reports_dir),
        build_dir = cfg.build_dir or defaults.build_dir,
        fuzzer_bin = cfg.fuzzer_bin or defaults.fuzzer_bin,
        fuzzer_time = fuzzer_time,
        fuzzer_profile = cfg.fuzzer_profile or run.fuzzer_profile,
        fuzzer_rss_limit_mb = cfg.fuzzer_rss_limit_mb or beez.env("FUZZER_RSS_LIMIT_MB"),
        mode = cfg.mode or run.mode or "script",
        needs_dict = run.needs_dict,
        log_prefix = cfg.log_prefix or run.log_prefix,
        fuzz_rev = cfg.fuzz_rev or defaults.fuzz_rev,
    }
end

return M
