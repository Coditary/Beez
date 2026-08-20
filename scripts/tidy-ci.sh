#!/usr/bin/env bash
set -euo pipefail

# Single clang-tidy pass for CI (replaces separate lint, analyze, and security tidy runs).
# Uses checks from .clang-tidy at the repository root.

BUILD_DIR="${1:-build}"
COMPDB_DIR="${BUILD_DIR}/build/Release"
COMPDB="${COMPDB_DIR}/compile_commands.json"
HEADER_FILTER="^${PWD}/(src|include|tests)/.*"
FAILED=0

if [[ ! -f "${COMPDB}" ]]; then
    echo "error: compile_commands.json not found at ${COMPDB}" >&2
    echo "Run 'make build' before clang-tidy." >&2
    exit 2
fi

echo "=== Running clang-tidy (CI combined pass) ==="
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
if [ "$FAILED" -ne 0 ]; then
    echo "=== clang-tidy failed ==="
    exit 1
fi

echo "=== clang-tidy complete ==="
