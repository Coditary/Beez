#!/usr/bin/env bash
# Shared libFuzzer settings for fuzz_lua_dsl (sourced by fuzz-*.sh).

fuzz_libfuzzer_args() {
  local profile="${FUZZER_PROFILE:-smoke}"
  # shellcheck disable=SC2034
  FUZZER_LIBFUZZER_ARGS=(
    -dict=tests/fuzz/lua_dsl.dict
    -detect_leaks=0
    -max_len=65536
    -timeout=5
    -use_value_profile=1
    -reduce_inputs=1
    -shrink=1
    -rss_limit_mb=0
    -artifact_prefix="${FUZZER_ARTIFACTS_DIR}/"
  )

  if [[ "${profile}" == "torture" ]]; then
    FUZZER_LIBFUZZER_ARGS+=(
      -entropic=1
      -len_control=0
      -cross_over_uniform_dist=1
    )
    if [[ -n "${FUZZER_JOBS:-}" ]]; then
      FUZZER_LIBFUZZER_ARGS+=(-jobs="${FUZZER_JOBS}")
    fi
  fi
}
