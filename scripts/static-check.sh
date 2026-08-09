#!/usr/bin/env bash
set -euo pipefail

# Standalone cppcheck pass for CI (no build required).

FAILED=0

echo "=== Running cppcheck ==="
if ! cppcheck --enable=warning,style,performance,portability --std=c++20 \
    -I include \
    --suppress=missingIncludeSystem \
    --suppress=missingInclude \
    --suppress=unusedFunction \
    --inline-suppr \
    --error-exitcode=1 \
    --quiet \
    src include 2>&1; then
    FAILED=1
fi

echo ""
if [ "$FAILED" -ne 0 ]; then
    echo "=== Static analysis failed ==="
    exit 1
fi

echo "=== Static analysis complete ==="
