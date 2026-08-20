#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

BUILD_DIR="${1:-build}"
BUILD_TYPE="${LINT_BUILD_TYPE:-Release}"
COMPDB_DIR="${BUILD_DIR}/build/${BUILD_TYPE}"
REPORTS_ROOT="${REPORTS_DIR:-report}"
CACHE_DIR="${REPORTS_ROOT}/lint/stale"
MANIFEST="${CACHE_DIR}/manifest.tsv"
CONTEXT_FILE="${CACHE_DIR}/context"
HEADER_FILTER="^${PWD}/(src|include|tests)/.*"

CMAKE_FILES=(
    CMakeLists.txt
    src/CMakeLists.txt
    src/app/CMakeLists.txt
    src/cli/CMakeLists.txt
    src/core/CMakeLists.txt
    src/logging/CMakeLists.txt
    src/plugins/CMakeLists.txt
    src/plugins/lua/CMakeLists.txt
    src/plugins/shell/CMakeLists.txt
    tests/CMakeLists.txt
    tests/unit/CMakeLists.txt
    tests/integration/CMakeLists.txt
    tests/system/CMakeLists.txt
    tests/fuzz/CMakeLists.txt
)

declare -A MANIFEST_MTIME=()
declare -A MANIFEST_SIZE=()
declare -A MANIFEST_STATUS=()

SKIPPED=0
CHECKED=0
FAILED=0

load_manifest() {
    if [[ ! -f "$MANIFEST" ]]; then
        return
    fi

    while IFS=$'\t' read -r path mtime size status; do
        [[ -z "$path" ]] && continue
        MANIFEST_MTIME["$path"]="$mtime"
        MANIFEST_SIZE["$path"]="$size"
        MANIFEST_STATUS["$path"]="$status"
    done <"$MANIFEST"
}

invalidate_cache_if_context_changed() {
    mkdir -p "$CACHE_DIR"

    local clang_tidy_stamp="0"
    local compdb_stamp="0"
    if [[ -f .clang-tidy ]]; then
        clang_tidy_stamp="$(stat -c '%Y' .clang-tidy)"
    fi
    if [[ -f "${COMPDB_DIR}/compile_commands.json" ]]; then
        compdb_stamp="$(stat -c '%Y' "${COMPDB_DIR}/compile_commands.json")"
    fi

    local context_key="${clang_tidy_stamp}:${compdb_stamp}:${HEADER_FILTER}"
    if [[ -f "$CONTEXT_FILE" ]] && [[ "$(<"$CONTEXT_FILE")" != "$context_key" ]]; then
        echo "Lint context changed; invalidating stale cache"
        rm -f "$MANIFEST"
        MANIFEST_MTIME=()
        MANIFEST_SIZE=()
        MANIFEST_STATUS=()
    fi

    printf '%s\n' "$context_key" >"$CONTEXT_FILE"
}

file_stamp() {
    stat -c '%Y %s' "$1"
}

should_skip_file() {
    local file="$1"

    if [[ "${LINT_FORCE:-0}" == "1" ]]; then
        return 1
    fi

    if [[ -z "${MANIFEST_STATUS[$file]+x}" ]]; then
        return 1
    fi

    if [[ "${MANIFEST_STATUS[$file]}" != "ok" ]]; then
        return 1
    fi

    local current_mtime current_size
    read -r current_mtime current_size < <(file_stamp "$file")
    if [[ "${MANIFEST_MTIME[$file]}" == "$current_mtime" &&
        "${MANIFEST_SIZE[$file]}" == "$current_size" ]]; then
        return 0
    fi

    return 1
}

clang_tidy_has_issue() {
    local file="$1"
    local tidy_output
    local file_has_issue=0

    mapfile -t tidy_output < <(
        clang-tidy -p "$COMPDB_DIR" "$file" \
            --header-filter="$HEADER_FILTER" \
            --use-color 2>&1 || true
    )

    for line in "${tidy_output[@]}"; do
        echo "$line"
        if [[ "$line" =~ /(src|include|tests)/.*(warning|error): ]]; then
            file_has_issue=1
        fi
    done

    return "$file_has_issue"
}

record_manifest_entry() {
    local file="$1"
    local status="$2"
    local mtime size

    read -r mtime size < <(file_stamp "$file")
    printf '%s\t%s\t%s\t%s\n' "$file" "$mtime" "$size" "$status" >>"$MANIFEST.tmp"
}

run_clang_tidy() {
    local file="$1"

    if should_skip_file "$file"; then
        echo "Skipping (fresh): $file"
        record_manifest_entry "$file" "ok"
        SKIPPED=$((SKIPPED + 1))
        return
    fi

    echo "Checking: $file"
    CHECKED=$((CHECKED + 1))

    if ! clang_tidy_has_issue "$file"; then
        record_manifest_entry "$file" "fail"
        FAILED=1
        return
    fi

    record_manifest_entry "$file" "ok"
}

run_cmake_format() {
    local file="$1"

    if should_skip_file "$file"; then
        echo "Skipping (fresh): $file"
        record_manifest_entry "$file" "ok"
        SKIPPED=$((SKIPPED + 1))
        return
    fi

    echo "Checking: $file"
    CHECKED=$((CHECKED + 1))

    if cmake-format --check "$file"; then
        record_manifest_entry "$file" "ok"
    else
        record_manifest_entry "$file" "fail"
        FAILED=1
    fi
}

main() {
    invalidate_cache_if_context_changed
    load_manifest

    : >"$MANIFEST.tmp"

    echo "=== Running clang-tidy (stale) ==="
    mapfile -t cxx_files < <(find src include tests \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) 2>/dev/null | sort)
    if [[ ${#cxx_files[@]} -eq 0 ]]; then
        echo "No C++ files found"
    else
        for file in "${cxx_files[@]}"; do
            run_clang_tidy "$file"
        done
    fi

    echo ""
    echo "=== Running cmake-format check (stale) ==="
    for file in "${CMAKE_FILES[@]}"; do
        run_cmake_format "$file"
    done

    mv "$MANIFEST.tmp" "$MANIFEST"

    echo ""
    echo "=== Lint stale summary ==="
    echo "Skipped: $SKIPPED"
    echo "Checked: $CHECKED"
    if [[ "$FAILED" -ne 0 ]]; then
        echo "Result: failed"
        exit 1
    fi

    echo "Result: ok"
}

main "$@"
