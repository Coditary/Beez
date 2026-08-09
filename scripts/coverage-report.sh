#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
REPORTS_DIR="${2:-report}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEBUG_BUILD_TREE="${ROOT_DIR}/${BUILD_DIR}/build/Debug"
REPORT_HTML="${ROOT_DIR}/${REPORTS_DIR}/coverage/index.html"
JSON_SUMMARY="${ROOT_DIR}/${REPORTS_DIR}/coverage/coverage-summary.json"
BADGE_JSON="${ROOT_DIR}/${REPORTS_DIR}/coverage/coverage-badge.json"
MIN_LINE_COVERAGE="${MIN_LINE_COVERAGE:-85}"

if [[ ! -d "${DEBUG_BUILD_TREE}" ]]; then
    echo "coverage test tree not found: ${DEBUG_BUILD_TREE}" >&2
    echo "Run make setup-coverage (or beez configure:coverage) first." >&2
    exit 2
fi

mkdir -p "${ROOT_DIR}/${REPORTS_DIR}/coverage"
cd "${DEBUG_BUILD_TREE}"

echo "=== Coverage summary (src/, minimum line coverage: ${MIN_LINE_COVERAGE}%) ==="
set +e
gcovr --gcov-executable 'llvm-cov gcov' \
    --root "${ROOT_DIR}" \
    --filter "${ROOT_DIR}/src/" \
    --gcov-ignore-parse-errors=suspicious_hits.warn \
    --json-summary "${JSON_SUMMARY}" \
    --fail-under-line "${MIN_LINE_COVERAGE}" \
    --print-summary .
coverage_status=$?
set -e

python3 "${ROOT_DIR}/scripts/coverage-badge-json.py" \
    "${JSON_SUMMARY}" "${BADGE_JSON}" "${MIN_LINE_COVERAGE}"

if [ "${coverage_status}" -ne 0 ]; then
    exit "${coverage_status}"
fi

echo ""
echo "=== HTML coverage report (src/ and tests/) ==="
gcovr --gcov-executable 'llvm-cov gcov' \
    --root "${ROOT_DIR}" \
    --filter "${ROOT_DIR}/src/" \
    --filter "${ROOT_DIR}/tests/" \
    --gcov-ignore-parse-errors=suspicious_hits.warn \
    --html-details "${REPORT_HTML}" .
