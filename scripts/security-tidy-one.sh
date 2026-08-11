#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "usage: security-tidy-one.sh <compile-db-dir> <file> [header-filter]" >&2
    exit 2
fi

COMPDB_DIR="$1"
FILE="$2"
HEADER_FILTER="${3:-(src|include|tests)/.*}"

if [[ ! -f "${COMPDB_DIR}/compile_commands.json" ]]; then
    echo "compile_commands.json not found under: ${COMPDB_DIR}" >&2
    exit 2
fi

mapfile -t tidy_output < <(
    clang-tidy -p "$COMPDB_DIR" "$FILE" \
        --header-filter="$HEADER_FILTER" \
        --checks='-*,clang-analyzer-security-*,cert-*,misc-security-*' \
        --use-color 2>&1 || true
)

file_has_issue=0
for line in "${tidy_output[@]}"; do
    echo "$line"
    if [[ "$line" =~ /(src|include|tests)/.*(warning|error): ]] && [[ ! "$line" =~ /_deps/ ]]; then
        file_has_issue=1
    fi
done

if [[ "$file_has_issue" -ne 0 ]]; then
    exit 1
fi

exit 0
