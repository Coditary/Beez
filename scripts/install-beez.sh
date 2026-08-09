#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Release}"
INSTALL_DIR="${BEEZ_INSTALL_DIR:-"$HOME/.local/bin"}"
SOURCE_BIN="${ROOT_DIR}/build/build/${BUILD_TYPE}/bin/beez"
TARGET="${INSTALL_DIR}/beez"
INSTALL_COMPLETION=0

# shellcheck source=scripts/completions/install-lib.sh
source "${ROOT_DIR}/scripts/completions/install-lib.sh"

usage() {
    cat <<EOF
Usage: install-beez.sh [--completion]

Install the beez binary and completion scripts.

Options:
  --completion   Also register shell completion in ~/.zshrc and ~/.bashrc
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --completion)
            INSTALL_COMPLETION=1
            shift
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [[ ! -x "$SOURCE_BIN" ]]; then
    echo "beez binary not found: ${SOURCE_BIN}" >&2
    echo "Build first: make build  (or make debug for Debug)" >&2
    exit 1
fi

mkdir -p "$INSTALL_DIR"
ln -sf "$SOURCE_BIN" "$TARGET"

install_beez_completion_files "${ROOT_DIR}/scripts/completions"
cp -f "${ROOT_DIR}/scripts/install-beez-completion.sh" "${BEEZ_DATA_DIR}/install-beez-completion.sh"
chmod +x "${BEEZ_DATA_DIR}/install-beez-completion.sh"

if [[ "$INSTALL_COMPLETION" -eq 1 ]]; then
    install_beez_completion_hooks
fi

echo "Installed beez:"
echo "  ${TARGET} -> ${SOURCE_BIN}"
echo ""
echo "Installed shell completion scripts:"
print_beez_completion_paths

if [[ "$INSTALL_COMPLETION" -eq 0 ]]; then
    echo ""
    echo "To enable tab completion in your shell, run:"
    echo "  beez --install-completion"
    echo "  # or: make install-beez-completion"
fi

case ":${PATH}:" in
    *":${INSTALL_DIR}:"*)
        if [[ "$INSTALL_COMPLETION" -eq 0 ]]; then
            echo ""
            echo "Open a new shell or run: hash -r"
        fi
        ;;
    *)
        echo ""
        echo "${INSTALL_DIR} is not on PATH."
        echo "Add to your shell rc (~/.bashrc or ~/.zshrc):"
        echo "  export PATH=\"${INSTALL_DIR}:\$PATH\""
        ;;
esac
