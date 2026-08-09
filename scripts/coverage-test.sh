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

if [[ ! -f "${DEBUG_BUILD_TREE}/.beez-coverage-configured" ]]; then
    echo "error: coverage stamp missing (${DEBUG_BUILD_TREE}/.beez-coverage-configured)." >&2
    echo "Run: make setup-coverage (or beez configure:coverage && beez build:coverage)" >&2
    exit 2
fi

if ! grep -qE 'BUILD_COVERAGE:(BOOL|UNINITIALIZED)=ON' "${DEBUG_BUILD_TREE}/CMakeCache.txt" 2>/dev/null; then
    echo "error: Debug build is not configured for coverage (BUILD_COVERAGE=OFF or stale after sanitize/tsan/fuzz)." >&2
    echo "Run: make setup-coverage (or beez configure:coverage && beez build:coverage)" >&2
    exit 2
fi

mkdir -p "${ROOT_DIR}/${REPORTS_DIR}/test"
find "${DEBUG_BUILD_TREE}" -name '*.gcda' -delete

cd "${DEBUG_BUILD_TREE}"
CTEST_JOBS="${CTEST_JOBS:-1}"
ctest -j"${CTEST_JOBS}" --output-on-failure 2>&1 | tee "${REPORT_FILE}"

if ! find "${DEBUG_BUILD_TREE}" -name '*.gcda' -print -quit | grep -q .; then
    echo "error: no .gcda files after ctest; coverage instrumentation data is missing" >&2
    echo "Rebuild with BUILD_COVERAGE=ON (beez configure:coverage && beez build:coverage) and rerun tests." >&2
    exit 1
fi

touch "${ROOT_DIR}/${REPORTS_DIR}/test/coverage-test-report.ok"
