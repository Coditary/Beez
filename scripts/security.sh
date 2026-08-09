#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
COMPDB_DIR="${BUILD_DIR}/build/Release"
COMPDB="${COMPDB_DIR}/compile_commands.json"
HEADER_FILTER='(src|include|tests)/.*'
FAILED=0

if [[ ! -f "${COMPDB}" ]]; then
    echo "error: compile_commands.json not found at ${COMPDB}" >&2
    echo "Run 'make setup build' before security analysis." >&2
    exit 2
fi

echo "=== Running security analysis ==="
echo ""
echo "--- clang-tidy security checks ---"
CXX_FILES=$(find src include -name "*.cpp" 2>/dev/null)
if [ -z "$CXX_FILES" ]; then
    echo "No C++ files found"
else
    while IFS= read -r file; do
        echo "Scanning: $file"
        mapfile -t tidy_output < <(
            clang-tidy -p "$COMPDB_DIR" "$file" \
                --header-filter="$HEADER_FILTER" \
                --checks='-*,clang-analyzer-security-*,cert-*,misc-security-*' \
                --use-color 2>&1 || true
        )

        file_has_issue=0
        for line in "${tidy_output[@]}"; do
            echo "$line"
            if [[ "$line" =~ /(src|include|tests)/.*(warning|error): ]]; then
                file_has_issue=1
            fi
        done

        if [ "$file_has_issue" -ne 0 ]; then
            FAILED=1
        fi
    done <<< "$CXX_FILES"
fi

echo ""
echo "--- cppcheck security checks ---"
if ! cppcheck --enable=warning,style,performance,portability \
    --std=c++20 \
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
    echo "=== Security analysis failed ==="
    exit 1
fi

echo "=== Security analysis complete ==="
