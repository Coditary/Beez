#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

BUILD_DIR="${BUILD_DIR:-build-release-check}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

validate_profile() {
    local platform="$1"
    local arch="$2"

    echo "=== Conan graph: ${platform}-${arch} ==="
    RELEASE_PLATFORM="${platform}" RELEASE_ARCH="${arch}" ./scripts/ci/release-conan-profile.sh >/dev/null
    conan graph info . \
        --lockfile-partial \
        -o "&:build_testing=False" \
        -pr "${ROOT_DIR}/.ci/release-conan-profile" \
        -pr:b "${ROOT_DIR}/.ci/release-conan-profile" \
        -s "build_type=${BUILD_TYPE}"
}

validate_profile linux x86_64
validate_profile linux aarch64
validate_profile macos x86_64
validate_profile macos aarch64
validate_profile windows x86_64
validate_profile windows aarch64

if [[ "${1:-}" == "--build" ]]; then
    echo "=== Local release build (host platform) ==="
    BUILD_DIR="${BUILD_DIR}" ./scripts/ci/release-build.sh
fi

echo "Release profile validation passed."
