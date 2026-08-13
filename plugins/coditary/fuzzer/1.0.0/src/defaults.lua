local M = {}

M.reports_dir = "report"
M.build_dir = "build"
M.debug_build_tree = "build/build/Debug"
M.fuzzer_bin = M.debug_build_tree .. "/fuzz/fuzz_lua_dsl"

M.corpus_glob = "tests/fuzz/corpus/lua_dsl/*.lua"
M.dict_file = "tests/fuzz/lua_dsl.dict"
M.fuzz_common_script = "scripts/fuzz-common.sh"

M.fuzz_rev = "1"

M.runs = {
    smoke = {
        scope = "smoke",
        script = "scripts/fuzz-smoke.sh",
        report_file = "report/fuzz/fuzz-smoke-report.txt",
        fuzzer_time_env = "FUZZER_TIME",
        fuzzer_time_default = "30",
        needs_dict = true,
        log_prefix = "[fuzz-smoke]",
        description = "Short fuzz run (FUZZER_TIME seconds, default 30)",
    },

    corpus = {
        scope = "corpus",
        script = "scripts/fuzz-corpus.sh",
        report_file = "report/fuzz/fuzz-corpus-report.txt",
        fuzzer_time = "60",
        needs_dict = true,
        log_prefix = "[fuzz-corpus]",
        description = "Longer fuzz run for corpus collection (60s)",
    },

    seed_verify = {
        scope = "verify",
        script = "scripts/fuzz-seed-verify.sh",
        report_file = "report/fuzz/fuzz-seed-verify-report.txt",
        needs_dict = false,
        log_prefix = "[fuzz-seed-verify]",
        description = "Run each committed fuzz seed through the harness once",
    },

    torture = {
        scope = "torture",
        script = "scripts/fuzz-torture.sh",
        report_file = "report/fuzz/fuzz-torture-report.txt",
        fuzzer_time_env = "FUZZER_TORTURE_TIME",
        fuzzer_time_default = "300",
        fuzzer_profile = "torture",
        needs_dict = true,
        log_prefix = "[fuzz-torture]",
        description = "Aggressive multi-minute fuzz run (FUZZER_TORTURE_TIME, default 300s)",
    },

    seeds_generate = {
        scope = "seeds",
        script = "scripts/generate-fuzz-seeds.sh",
        report_file = "report/fuzz/fuzz-seeds.ok",
        mode = "direct",
        log_prefix = "[fuzz-seeds]",
        description = "Regenerate field-matrix fuzz seeds from fixtures",
        extra_inputs = {
            "tests/system/fixtures/**",
            "scripts/generate-fuzz-seeds.sh",
        },
    },
}

M.fuzz_step_runs = {
    ["fuzz:smoke"] = "smoke",
    ["fuzz:corpus"] = "corpus",
    ["fuzz:seed-verify"] = "seed_verify",
    ["fuzz:torture"] = "torture",
    ["fuzz:seeds"] = "seeds_generate",
}

return M
