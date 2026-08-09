#!/usr/bin/env bash
set -euo pipefail

# Install a pinned LLVM toolchain on Debian/Ubuntu (CI runners).
# Default: LLVM 22 to match current Fedora dev images and apt.llvm.org noble packages.

LLVM_VERSION="${LLVM_VERSION:-22}"
UBUNTU_CODENAME="${UBUNTU_CODENAME:-noble}"

if ! command -v apt-get >/dev/null 2>&1; then
    echo "ci-install-llvm.sh supports Debian/Ubuntu only (apt-get not found)" >&2
    exit 1
fi

wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
    | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc >/dev/null

echo "deb http://apt.llvm.org/${UBUNTU_CODENAME}/ llvm-toolchain-${UBUNTU_CODENAME}-${LLVM_VERSION} main" \
    | sudo tee "/etc/apt/sources.list.d/llvm-${LLVM_VERSION}.list" >/dev/null

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    "clang-${LLVM_VERSION}" \
    "clang-tidy-${LLVM_VERSION}" \
    "clang-format-${LLVM_VERSION}" \
    "clang-tools-${LLVM_VERSION}" \
    "llvm-${LLVM_VERSION}"

register_alt() {
    local name="$1"
    local path="$2"
    sudo update-alternatives --install "/usr/bin/${name}" "${name}" "${path}" 100
}

register_alt clang "/usr/bin/clang-${LLVM_VERSION}"
register_alt clang++ "/usr/bin/clang++-${LLVM_VERSION}"
register_alt clang-tidy "/usr/bin/clang-tidy-${LLVM_VERSION}"
register_alt clang-format "/usr/bin/clang-format-${LLVM_VERSION}"
register_alt llvm-cov "/usr/bin/llvm-cov-${LLVM_VERSION}"

echo "=== LLVM toolchain ${LLVM_VERSION} ==="
clang --version | head -n1
clang-tidy --version | head -n1
clang-format --version | head -n1
