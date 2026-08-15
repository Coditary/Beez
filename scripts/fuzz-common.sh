#!/usr/bin/env bash
# Shared libFuzzer settings for fuzz_lua_dsl (sourced by fuzz-*.sh).
#
# Env:
#   FUZZER_RSS_LIMIT_MB  Per-process RSS cap in MiB (default: 8192; 0 = unlimited)
#
# Fuzzing always runs as a single libFuzzer process (no -jobs / -workers).

resolve_fuzzer_bin() {
    local build_dir="${1:-build}"
    local candidate
    for candidate in \
        "${build_dir}/build/Debug/fuzz/fuzz_lua_dsl" \
        "${build_dir}/build/Fuzz/fuzz/fuzz_lua_dsl"
    do
        if [[ -x "${candidate}" ]]; then
            echo "${candidate}"
            return 0
        fi
    done

    echo "ERROR: fuzz_lua_dsl not found under ${build_dir}/build/{Debug/fuzz,Fuzz/fuzz}/" >&2
    return 1
}

fuzz_libfuzzer_args() {
  local profile="${FUZZER_PROFILE:-smoke}"
  local rss_limit_mb="${FUZZER_RSS_LIMIT_MB:-8192}"
  # shellcheck disable=SC2034
  FUZZER_LIBFUZZER_ARGS=(
    -dict=tests/fuzz/lua_dsl.dict
    -detect_leaks=0
    -max_len=65536
    -timeout=5
    -use_value_profile=1
    -reduce_inputs=1
    -shrink=1
    -rss_limit_mb="${rss_limit_mb}"
    -artifact_prefix="${FUZZER_ARTIFACTS_DIR}/"
  )

  if [[ "${profile}" == "torture" ]]; then
    FUZZER_LIBFUZZER_ARGS+=(
      -entropic=1
      -len_control=0
      -cross_over_uniform_dist=1
    )
  fi
}
