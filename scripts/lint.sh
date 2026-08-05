#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
COMPDB_DIR="${BUILD_DIR}/build/Release"
HEADER_FILTER='(src|include|tests)/.*'
FAILED=0

echo "=== Running clang-tidy ==="
CXX_FILES=$(find src include tests -name "*.cpp" -o -name "*.hpp" -o -name "*.h" 2>/dev/null)
if [ -z "$CXX_FILES" ]; then
    echo "No C++ files found"
else
    while IFS= read -r file; do
        echo "Checking: $file"
        mapfile -t tidy_output < <(
            clang-tidy -p "$COMPDB_DIR" "$file" \
                --header-filter="$HEADER_FILTER" \
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
echo "=== Running cmake-format check ==="
if ! cmake-format --check \
    CMakeLists.txt \
    src/CMakeLists.txt \
    src/app/CMakeLists.txt \
    src/cli/CMakeLists.txt \
    src/core/CMakeLists.txt \
    src/logging/CMakeLists.txt \
    src/plugins/CMakeLists.txt \
    src/plugins/lua/CMakeLists.txt \
    src/plugins/shell/CMakeLists.txt \
    tests/CMakeLists.txt \
    tests/unit/CMakeLists.txt \
    tests/integration/CMakeLists.txt \
    tests/system/CMakeLists.txt \
    tests/fuzz/CMakeLists.txt; then
    FAILED=1
fi

echo ""
if [ "$FAILED" -ne 0 ]; then
    echo "=== Lint failed ==="
    exit 1
fi

echo "=== Lint complete ==="
