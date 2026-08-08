#!/usr/bin/env bash
set -euo pipefail

INSTALL_DIR="${BEEZ_INSTALL_DIR:-"$HOME/.local/bin"}"
TARGET="${INSTALL_DIR}/beez"

if [[ -L "$TARGET" ]]; then
    rm "$TARGET"
    echo "Removed ${TARGET}"
    exit 0
fi

if [[ -e "$TARGET" ]]; then
    echo "Refusing to remove non-symlink: ${TARGET}" >&2
    exit 1
fi

echo "beez is not installed at ${TARGET}"
