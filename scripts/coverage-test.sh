#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
REPORTS_DIR="${2:-report}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEBUG_BUILD_TREE="${ROOT_DIR}/${BUILD_DIR}/build/Debug"
REPORT_FILE="${ROOT_DIR}/${REPORTS_DIR}/test/coverage-test-report.txt"

if [[ ! -d "${DEBUG_BUILD_TREE}" ]]; then
    echo "coverage test tree not found: ${DEBUG_BUILD_TREE}" >&2
    echo "Run make setup-coverage (or beez configure:coverage) first." >&2
    exit 2
fi

mkdir -p "${ROOT_DIR}/${REPORTS_DIR}/test"
find "${DEBUG_BUILD_TREE}" -name '*.gcda' -delete

cd "${DEBUG_BUILD_TREE}"
ctest -j1 --output-on-failure 2>&1 | tee "${REPORT_FILE}"
