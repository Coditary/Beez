#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
COMPDB_DIR="${BUILD_DIR}/build/Release"
HEADER_FILTER='(src|include)/.*'
TIDY_CHECKS='-*,clang-analyzer-*,bugprone-*,cppcoreguidelines-*,performance-*'
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
    src 2>&1; then
    FAILED=1
fi

echo ""
echo "=== Running clang-tidy analysis ==="
CXX_FILES=$(find src -name "*.cpp" 2>/dev/null)
if [ -z "$CXX_FILES" ]; then
    echo "No C++ files found"
else
    while IFS= read -r file; do
        echo "Analyzing: $file"
        mapfile -t tidy_output < <(
            clang-tidy -p "$COMPDB_DIR" "$file" \
                --header-filter="$HEADER_FILTER" \
                --checks="$TIDY_CHECKS" \
                --use-color 2>&1 || true
        )

        file_has_issue=0
        for line in "${tidy_output[@]}"; do
            echo "$line"
            if [[ "$line" =~ ^(src|include)/.*(warning|error): ]]; then
                file_has_issue=1
            fi
        done

        if [ "$file_has_issue" -ne 0 ]; then
            FAILED=1
        fi
    done <<< "$CXX_FILES"
fi

echo ""
if [ "$FAILED" -ne 0 ]; then
    echo "=== Analysis failed ==="
    exit 1
fi

echo "=== Analysis complete ==="
