#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
FUZZER_TIME="${FUZZER_TIME:-30}"
REPORTS_DIR="${REPORTS_DIR:-report}"
FUZZER_BIN="${BUILD_DIR}/build/Debug/fuzz/fuzz_lua_dsl"
FUZZER_CORPUS_DIR="${REPORTS_DIR}/fuzz/corpus/lua_dsl"
FUZZER_ARTIFACTS_DIR="${REPORTS_DIR}/fuzz/artifacts"
SEED_DIR="tests/fuzz/corpus/lua_dsl"

# shellcheck source=scripts/fuzz-common.sh
source "$(dirname "${BASH_SOURCE[0]}")/fuzz-common.sh"
fuzz_libfuzzer_args

if ! compgen -G "${SEED_DIR}/*.lua" > /dev/null; then
    echo "ERROR: No fuzz seeds in ${SEED_DIR}/*.lua" >&2
    exit 1
fi

rm -rf "${FUZZER_CORPUS_DIR}" "${FUZZER_ARTIFACTS_DIR}"
mkdir -p "${FUZZER_CORPUS_DIR}" "${FUZZER_ARTIFACTS_DIR}" "${REPORTS_DIR}/fuzz"
cp "${SEED_DIR}"/*.lua "${FUZZER_CORPUS_DIR}/"

echo "=== Running fuzz_lua_dsl for ${FUZZER_TIME}s (invalid Lua input is expected) ==="
ASAN_OPTIONS=detect_leaks=0 \
    "${FUZZER_BIN}" "${FUZZER_CORPUS_DIR}" \
    "${FUZZER_LIBFUZZER_ARGS[@]}" \
    -max_total_time="${FUZZER_TIME}" \
    -print_final_stats=1 \
    2>&1 | tee "${REPORTS_DIR}/fuzz/fuzz-smoke-report.txt"
