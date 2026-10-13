#!/usr/bin/env bash
# Clone Coditary/Shared-Cpp to the path expected by CMakeLists.txt on CI runners.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_DIR="$(cd "${ROOT_DIR}/../.." && pwd)/shared/Shared-Cpp"
REPO_URL="${SHARED_CPP_REPO:-https://github.com/Coditary/Shared-Cpp.git}"
REF="${SHARED_CPP_REF:-main}"

if [ -f "${TARGET_DIR}/cmake/CoditarySharedCpp.cmake" ]; then
    echo "Shared-Cpp already present at ${TARGET_DIR}"
    exit 0
fi

mkdir -p "$(dirname "${TARGET_DIR}")"
echo "Cloning Shared-Cpp (${REF}) into ${TARGET_DIR}"
git clone --depth 1 --branch "${REF}" "${REPO_URL}" "${TARGET_DIR}"

if [ ! -f "${TARGET_DIR}/cmake/CoditarySharedCpp.cmake" ]; then
    echo "ERROR: Shared-Cpp checkout is missing cmake/CoditarySharedCpp.cmake" >&2
    exit 1
fi
