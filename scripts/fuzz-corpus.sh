#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
FUZZER_TIME="${FUZZER_TIME:-60}"
REPORTS_DIR="${REPORTS_DIR:-report}"
FUZZER_BIN="${BUILD_DIR}/build/Debug/fuzz/fuzz_lua_dsl"
FUZZER_CORPUS_DIR="${REPORTS_DIR}/fuzz/corpus/lua_dsl"
FUZZER_ARTIFACTS_DIR="${REPORTS_DIR}/fuzz/artifacts"
SEED_DIR="tests/fuzz/corpus/lua_dsl"

if ! compgen -G "${SEED_DIR}/*.lua" > /dev/null; then
    echo "ERROR: No fuzz seeds in ${SEED_DIR}/*.lua" >&2
    exit 1
fi

rm -rf "${REPORTS_DIR}/fuzz/corpus" "${FUZZER_ARTIFACTS_DIR}"
mkdir -p "${FUZZER_CORPUS_DIR}" "${FUZZER_ARTIFACTS_DIR}" "${REPORTS_DIR}/fuzz"
cp "${SEED_DIR}"/*.lua "${FUZZER_CORPUS_DIR}/"

echo "=== Running fuzz_lua_dsl for ${FUZZER_TIME}s (corpus collection) ==="
ASAN_OPTIONS=detect_leaks=0 \
    "${FUZZER_BIN}" "${FUZZER_CORPUS_DIR}" \
    -dict=tests/fuzz/lua_dsl.dict \
    -detect_leaks=0 \
    -max_total_time="${FUZZER_TIME}" \
    -rss_limit_mb=0 \
    -artifact_prefix="${FUZZER_ARTIFACTS_DIR}/" \
    2>&1 | tee "${REPORTS_DIR}/fuzz/fuzz-corpus-report.txt"
