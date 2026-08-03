#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
COMPDB_DIR="${BUILD_DIR}/build/Release"

echo "=== Running security analysis ==="
echo ""
echo "--- clang-tidy security checks ---"
CXX_FILES=$(find src include -name "*.cpp" 2>/dev/null)
if [ -z "$CXX_FILES" ]; then
    echo "No C++ files found"
else
    echo "$CXX_FILES" | while read -r file; do
        echo "Scanning: $file"
        clang-tidy -p "$COMPDB_DIR" "$file" \
            --checks='-*,clang-analyzer-security-*,cert-*,misc-security-*' \
            --use-color || true
    done
fi

echo ""
echo "--- cppcheck security checks ---"
cppcheck --enable=warning,style,performance,portability \
    --std=c++20 --suppress=missingIncludeSystem \
    --output-file=security-report.txt \
    src include

cat security-report.txt

echo ""
echo "=== Security analysis complete ==="
