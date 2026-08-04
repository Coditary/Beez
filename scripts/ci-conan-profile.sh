#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CXX="${CXX:-clang++}"
PROFILE_PATH="${ROOT_DIR}/.ci/conan-profile"

CLANG_MAJOR="$("$CXX" --version | sed -n 's/.*clang version \([0-9]*\).*/\1/p')"
if [ -z "$CLANG_MAJOR" ]; then
    echo "ERROR: Could not detect Clang version from $CXX" >&2
    exit 1
fi

mkdir -p "${ROOT_DIR}/.ci"

cat > "${PROFILE_PATH}" <<EOF
[settings]
arch=x86_64
build_type=Release
compiler=clang
compiler.version=${CLANG_MAJOR}
compiler.libcxx=libstdc++11
compiler.cppstd=20
os=Linux
EOF

echo "${PROFILE_PATH}"
