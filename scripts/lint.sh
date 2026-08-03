#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
COMPDB_DIR="${BUILD_DIR}/build/Release"

echo "=== Running clang-tidy ==="
CXX_FILES=$(find src include tests -name "*.cpp" -o -name "*.hpp" -o -name "*.h" 2>/dev/null)
if [ -z "$CXX_FILES" ]; then
    echo "No C++ files found"
else
    echo "$CXX_FILES" | while read -r file; do
        echo "Checking: $file"
        clang-tidy -p "$COMPDB_DIR" "$file" --use-color --quiet 2>&1 | grep -E "(warning|error):" || true
    done
fi

echo ""
echo "=== Running cmake-format check ==="
cmake-format --check \
    CMakeLists.txt \
    src/CMakeLists.txt \
    src/app/CMakeLists.txt \
    src/core/CMakeLists.txt \
    src/plugins/CMakeLists.txt \
    src/plugins/lua/CMakeLists.txt \
    src/plugins/shell/CMakeLists.txt \
    tests/CMakeLists.txt \
    tests/unit/CMakeLists.txt \
    tests/integration/CMakeLists.txt \
    fuzz/CMakeLists.txt 2>&1 || true

echo ""
echo "=== Lint complete ==="
