local defaults = require("src.defaults")
local step_config = require("src.step_config")

local runner = require("src.runner")

local function build_inputs(run_name)
    local run = defaults.runs[run_name]
    local inputs = {
        defaults.fuzzer_bin,
        defaults.corpus_glob,
    }

    if run.needs_dict then
        inputs[#inputs + 1] = defaults.dict_file
        inputs[#inputs + 1] = defaults.fuzz_common_script
    end

    inputs[#inputs + 1] = run.script

    if run.extra_inputs ~= nil then
        for _, pattern in ipairs(run.extra_inputs) do
            inputs[#inputs + 1] = pattern
        end
    end

    return inputs
end

local plugin_steps = {}

for step_name, run_name in pairs(defaults.fuzz_step_runs) do
    local run = defaults.runs[run_name]

    plugin_steps[step_name] = {
        phase = "fuzz",
        scope = run.scope,
        input = build_inputs(run_name),
        output = { run.report_file },
        description = run.description,
        config = step_config.run_defaults(run_name),
        run = function(ctx)
            return runner.run(ctx, step_name)
        end,
    }
end

-- Beez fuzzer plugin
--
-- Steps (configure/build via coditary/conan: configure:fuzzer, build:fuzzer):
--   fuzz:smoke        — short libFuzzer run (FUZZER_TIME, default 30s)
--   fuzz:corpus       — 60s corpus collection run
--   fuzz:seed-verify  — run each seed once
--   fuzz:torture      — long run (FUZZER_TORTURE_TIME, default 300s)
--   fuzz:seeds        — regenerate seeds (scope seeds, not in default workflows)
--
-- Env: FUZZER_TIME, FUZZER_TORTURE_TIME, REPORTS_DIR
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/fuzzer",
--             path = "./plugins/coditary/fuzzer",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("fuzzer", {
    version = "1.0.0",
    description = "libFuzzer smoke, corpus, seed-verify, and torture runs",
    organization = "coditary",
    steps = plugin_steps,
})
