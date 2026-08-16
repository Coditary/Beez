#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PROFILE_PATH="${ROOT_DIR}/.ci/release-conan-profile"

PLATFORM="${RELEASE_PLATFORM:-linux}"
ARCH="${RELEASE_ARCH:-x86_64}"
BUILD_TYPE="${RELEASE_BUILD_TYPE:-Release}"

case "${ARCH}" in
    x86_64) CONAN_ARCH="x86_64" ;;
    aarch64) CONAN_ARCH="armv8" ;;
    *)
        echo "ERROR: unsupported RELEASE_ARCH=${ARCH}" >&2
        exit 1
        ;;
esac

mkdir -p "${ROOT_DIR}/.ci"

case "${PLATFORM}" in
    linux)
        CXX="${CXX:-clang++}"
        CLANG_MAJOR="$("$CXX" --version | sed -n 's/.*clang version \([0-9]*\).*/\1/p')"
        if [ -z "${CLANG_MAJOR}" ]; then
            echo "ERROR: Could not detect Clang version from ${CXX}" >&2
            exit 1
        fi
        cat > "${PROFILE_PATH}" <<EOF
[settings]
arch=${CONAN_ARCH}
build_type=${BUILD_TYPE}
compiler=clang
compiler.version=${CLANG_MAJOR}
compiler.libcxx=libstdc++11
compiler.cppstd=20
os=Linux
EOF
        ;;
    macos)
        CXX="${CXX:-clang++}"
        CLANG_MAJOR="$("$CXX" --version | sed -n 's/.*clang version \([0-9]*\).*/\1/p')"
        if [ -z "${CLANG_MAJOR}" ]; then
            echo "ERROR: Could not detect Apple Clang version from ${CXX}" >&2
            exit 1
        fi
        cat > "${PROFILE_PATH}" <<EOF
[settings]
arch=${CONAN_ARCH}
build_type=${BUILD_TYPE}
compiler=apple-clang
compiler.version=${CLANG_MAJOR}
compiler.libcxx=libc++
compiler.cppstd=20
os=Macos
EOF
        ;;
    windows)
        MSVC_VERSION="${MSVC_VERSION:-194}"
        VS_VERSION="${VS_VERSION:-18}"
        cat > "${PROFILE_PATH}" <<EOF
[settings]
arch=${CONAN_ARCH}
build_type=${BUILD_TYPE}
compiler=msvc
compiler.version=${MSVC_VERSION}
compiler.cppstd=20
compiler.runtime=dynamic
os=Windows

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.microsoft.msbuild:vs_version=${VS_VERSION}
EOF
        ;;
    *)
        echo "ERROR: unsupported RELEASE_PLATFORM=${PLATFORM}" >&2
        exit 1
        ;;
esac

echo "${PROFILE_PATH}"
