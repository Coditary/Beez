#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

export CC="${CC:-clang}"
export CXX="${CXX:-clang++}"
export FUZZER_TIME="${FUZZER_TIME:-30}"
export STRICT_CI=1
export CONAN_PROFILE="$("${ROOT_DIR}/scripts/ci-conan-profile.sh")"

run_step() {
    local name="$1"
    shift
    echo ""
    echo "================================================================"
    echo "  ${name}"
    echo "================================================================"
    "$@"
}

echo "=== Beez CI pipeline ==="
echo "Compiler: $($CXX --version | head -n1)"
echo "Conan profile: ${CONAN_PROFILE}"
echo "Fuzzer time: ${FUZZER_TIME}s"
echo ""
echo "Note: CI runs these steps in parallel on GitHub Actions."
echo "This script runs them sequentially for local reproduction."

run_step "Format check" make format-check
run_step "Static check (cppcheck)" make static-check
run_step "Build (Release)" make build
run_step "Tests" make test
run_step "Clang-tidy (CI combined pass)" make tidy-ci
run_step "Coverage" make coverage
run_step "Sanitizer (ASan/UBSan)" make sanitize
run_step "Fuzzer smoke test" make fuzzer-smoke

echo ""
echo "=== CI pipeline passed ==="
echo "Run 'make tsan' separately for ThreadSanitizer (parallel in GitHub Actions)."
