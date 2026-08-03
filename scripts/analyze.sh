#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
COMPDB_DIR="${BUILD_DIR}/build/Release"

echo "=== Running cppcheck ==="
cppcheck --enable=all --std=c++20 --suppress=missingIncludeSystem \
    --inline-suppr \
    --output-file=cppcheck-report.txt \
    src include

echo "cppcheck report written to cppcheck-report.txt"
cat cppcheck-report.txt

echo ""
echo "=== Running clang-tidy analysis ==="
CXX_FILES=$(find src include -name "*.cpp" 2>/dev/null)
if [ -z "$CXX_FILES" ]; then
    echo "No C++ files found"
else
    echo "$CXX_FILES" | while read -r file; do
        echo "Analyzing: $file"
        clang-tidy -p "$COMPDB_DIR" "$file" \
            --checks='-*,clang-analyzer-*,bugprone-*,cppcoreguidelines-*,performance-*' \
            --use-color || true
    done
fi

echo ""
echo "=== Analysis complete ==="
