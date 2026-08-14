#!/usr/bin/env bash
set -euo pipefail

# Install ReqPack (rqp) into ~/.local/bin for CI and local bootstrap.

INSTALL_DIR="${REQPACK_INSTALL_DIR:-${HOME}/.local/bin}"

if command -v rqp >/dev/null 2>&1; then
    echo "rqp already on PATH: $(command -v rqp)"
    rqp --version
    exit 0
fi

mkdir -p "${INSTALL_DIR}"
export PATH="${INSTALL_DIR}:${PATH}"

curl -fsSL https://raw.githubusercontent.com/Coditary/ReqPack/main/install.sh | sh

if ! command -v rqp >/dev/null 2>&1; then
    echo "ci-install-reqpack.sh: rqp not found after install.sh" >&2
    exit 1
fi

echo "Installed rqp to $(command -v rqp)"
rqp --version
