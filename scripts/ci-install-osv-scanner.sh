#!/usr/bin/env bash
set -euo pipefail

# Install a pinned osv-scanner binary into ~/.local/bin (CI and local bootstrap).

OSV_SCANNER_VERSION="${OSV_SCANNER_VERSION:-1.9.2}"
INSTALL_DIR="${OSV_SCANNER_INSTALL_DIR:-${HOME}/.local/bin}"
ARCH="$(uname -m)"

case "${ARCH}" in
    x86_64) OSV_ARCH="amd64" ;;
    aarch64 | arm64) OSV_ARCH="arm64" ;;
    *)
        echo "ci-install-osv-scanner.sh: unsupported architecture: ${ARCH}" >&2
        exit 1
        ;;
esac

OSV_OS="linux"
if [[ "$(uname -s)" != "Linux" ]]; then
    OSV_OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
fi

ASSET="osv-scanner_${OSV_OS}_${OSV_ARCH}"
URL="https://github.com/google/osv-scanner/releases/download/v${OSV_SCANNER_VERSION}/${ASSET}"

mkdir -p "${INSTALL_DIR}"
TMP="$(mktemp)"
curl -sSfL "${URL}" -o "${TMP}"
install -m 0755 "${TMP}" "${INSTALL_DIR}/osv-scanner"
rm -f "${TMP}"

echo "Installed osv-scanner ${OSV_SCANNER_VERSION} to ${INSTALL_DIR}/osv-scanner"
"${INSTALL_DIR}/osv-scanner" --version
