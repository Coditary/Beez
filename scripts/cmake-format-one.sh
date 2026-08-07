#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: cmake-format-one.sh <file> [--apply]" >&2
    exit 2
fi

FILE="$1"
MODE="${2:-check}"

if [[ "$MODE" == "--apply" ]]; then
    cmake-format -i "$FILE"
    exit 0
fi

cmake-format --check "$FILE"
