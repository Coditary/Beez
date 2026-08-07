#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Release}"
INSTALL_DIR="${BEEZ_INSTALL_DIR:-"$HOME/.local/bin"}"
SOURCE_BIN="${ROOT_DIR}/build/build/${BUILD_TYPE}/bin/beez"
TARGET="${INSTALL_DIR}/beez"

if [[ ! -x "$SOURCE_BIN" ]]; then
    echo "beez binary not found: ${SOURCE_BIN}" >&2
    echo "Build first: make build  (or make debug for Debug)" >&2
    exit 1
fi

mkdir -p "$INSTALL_DIR"
ln -sf "$SOURCE_BIN" "$TARGET"

echo "Installed beez:"
echo "  ${TARGET} -> ${SOURCE_BIN}"

case ":${PATH}:" in
    *":${INSTALL_DIR}:"*)
        echo ""
        echo "Open a new shell or run: hash -r"
        echo "Then from the Beez repo root: beez build"
        ;;
    *)
        echo ""
        echo "${INSTALL_DIR} is not on PATH."
        echo "Add to your shell rc (~/.bashrc or ~/.zshrc):"
        echo "  export PATH=\"${INSTALL_DIR}:\$PATH\""
        ;;
esac
