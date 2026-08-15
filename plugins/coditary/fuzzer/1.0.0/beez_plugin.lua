local defaults = require("src.defaults")

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
        phase = run.phase or "test",
        scope = run.scope,
        input = build_inputs(run_name),
        output = { run.report_file },
        description = run.description,
        config = {
            log_prefix = run.log_prefix,
        },
        run = function(ctx)
            return runner.run(ctx, step_name)
        end,
    }
end

plugin("fuzzer", {
    version = "1.0.0",
    description = "libFuzzer smoke, corpus, seed-verify, and torture runs",
    organization = "coditary",

    config = {
        defaults = {
            fuzz_rev = defaults.fuzz_rev,
            reports_dir = defaults.reports_dir,
            build_dir = defaults.build_dir,
            fuzzer_bin = defaults.fuzzer_bin,
        },
    },

    steps = plugin_steps,
})
