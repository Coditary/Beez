#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_TESTING="${BUILD_TESTING:-OFF}"

export CC="${CC:-clang}"
export CXX="${CXX:-clang++}"

PROFILE_PATH="$(./scripts/ci/release-conan-profile.sh)"
export CONAN_PROFILE="${PROFILE_PATH}"

echo "Using Conan profile at ${CONAN_PROFILE}"
cat "${CONAN_PROFILE}"

conan install . \
    --output-folder="${BUILD_DIR}" \
    --build=missing \
    --lockfile-partial \
    -o "&:build_testing=False" \
    -s "build_type=${BUILD_TYPE}" \
    -pr "${CONAN_PROFILE}" \
    -pr:b "${CONAN_PROFILE}"

cmake --preset "conan-release" \
    -DBUILD_TESTING="${BUILD_TESTING}" \
    -DBUILD_FUZZER=OFF \
    -DBUILD_COVERAGE=OFF \
    -DBUILD_CACHE=ON

cmake --build --preset conan-release --target beez --parallel

BINARY_PATH="${BUILD_DIR}/build/${BUILD_TYPE}/bin/beez"
if [ ! -x "${BINARY_PATH}" ]; then
    echo "ERROR: release binary not found at ${BINARY_PATH}" >&2
    exit 1
fi

echo "Built ${BINARY_PATH}"
