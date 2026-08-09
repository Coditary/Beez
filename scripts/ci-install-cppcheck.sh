#!/usr/bin/env bash
set -euo pipefail

# Install a pinned cppcheck release into ~/.local (CI and local bootstrap).

CPPCHECK_VERSION="${CPPCHECK_VERSION:-2.21.1}"
INSTALL_DIR="${CPPCHECK_INSTALL_DIR:-${HOME}/.local}"
ARCH="$(uname -m)"

if [[ ! "${CPPCHECK_VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "ci-install-cppcheck.sh: invalid CPPCHECK_VERSION: ${CPPCHECK_VERSION}" >&2
    echo "Expected a pinned release such as 2.21.1." >&2
    exit 1
fi

case "${ARCH}" in
    x86_64 | aarch64 | arm64) ;;
    *)
        echo "ci-install-cppcheck.sh: unsupported architecture: ${ARCH}" >&2
        exit 1
        ;;
esac

if ! command -v cmake >/dev/null; then
    echo "ci-install-cppcheck.sh: cmake is required to build cppcheck" >&2
    exit 1
fi

if ! command -v g++ >/dev/null && ! command -v clang++ >/dev/null; then
    if command -v apt-get >/dev/null; then
        sudo apt-get install -y --no-install-recommends g++
    else
        echo "ci-install-cppcheck.sh: a C++ compiler (g++ or clang++) is required" >&2
        exit 1
    fi
fi

SRC_DIR="$(mktemp -d)"
trap 'rm -rf "${SRC_DIR}"' EXIT

URL="https://github.com/cppcheck-opensource/cppcheck/archive/refs/tags/${CPPCHECK_VERSION}.tar.gz"
if ! curl -sSfL "${URL}" | tar xz -C "${SRC_DIR}" --strip-components=1; then
    echo "ci-install-cppcheck.sh: failed to download cppcheck ${CPPCHECK_VERSION}" >&2
    echo "URL: ${URL}" >&2
    exit 1
fi

cmake -S "${SRC_DIR}" -B "${SRC_DIR}/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DBUILD_GUI=OFF

cmake --build "${SRC_DIR}/build" -j"$(nproc)"
cmake --install "${SRC_DIR}/build"

if [[ ! -x "${INSTALL_DIR}/bin/cppcheck" ]]; then
    echo "ci-install-cppcheck.sh: install did not produce ${INSTALL_DIR}/bin/cppcheck" >&2
    exit 1
fi

echo "Installed cppcheck ${CPPCHECK_VERSION} to ${INSTALL_DIR}/bin/cppcheck"
"${INSTALL_DIR}/bin/cppcheck" --version
