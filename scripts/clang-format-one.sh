#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: clang-format-one.sh <file> [--apply]" >&2
    exit 2
fi

FILE="$1"
MODE="${2:-check}"

if [[ "$MODE" == "--apply" ]]; then
    clang-format -i "$FILE"
    exit 0
fi

clang-format --dry-run --Werror "$FILE"
