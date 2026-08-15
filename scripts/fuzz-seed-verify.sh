#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
REPORTS_DIR="${REPORTS_DIR:-report}"
SEED_DIR="tests/fuzz/corpus/lua_dsl"
REPORT="${REPORTS_DIR}/fuzz/fuzz-seed-verify-report.txt"

# shellcheck source=scripts/fuzz-common.sh
source "$(dirname "${BASH_SOURCE[0]}")/fuzz-common.sh"
FUZZER_BIN="$(resolve_fuzzer_bin "${BUILD_DIR}")"

if ! compgen -G "${SEED_DIR}/*.lua" > /dev/null; then
  echo "ERROR: No fuzz seeds in ${SEED_DIR}/*.lua" >&2
  exit 1
fi

if [[ ! -x "${FUZZER_BIN}" ]]; then
  echo "ERROR: Fuzzer binary not found: ${FUZZER_BIN}" >&2
  exit 1
fi

mkdir -p "${REPORTS_DIR}/fuzz"
: >"${REPORT}"

seed_count=0
for seed in "${SEED_DIR}"/*.lua; do
  seed_count=$((seed_count + 1))
  printf 'verify seed %s\n' "$(basename "${seed}")" >>"${REPORT}"
  ASAN_OPTIONS=detect_leaks=0 \
    "${FUZZER_BIN}" "${seed}" \
    -dict=tests/fuzz/lua_dsl.dict \
    -detect_leaks=0 \
    -runs=1 \
    -max_len=65536 \
    >>"${REPORT}" 2>&1
done

echo "Verified ${seed_count} fuzz seeds (see ${REPORT})"
