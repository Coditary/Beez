# syntax=docker/dockerfile:1.7

ARG UBUNTU_VERSION=24.04

FROM --platform=$BUILDPLATFORM ubuntu:${UBUNTU_VERSION} AS build

ARG DEBIAN_FRONTEND=noninteractive
ARG BEEZ_VERSION=1.0.2
ARG LLVM_VERSION=22
ARG UBUNTU_CODENAME=noble

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        curl \
        git \
        ninja-build \
        pipx \
        pkg-config \
        python3 \
        python3-venv \
        wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO- "https://apt.llvm.org/llvm-snapshot.gpg.key" \
        | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc >/dev/null \
    && echo "deb http://apt.llvm.org/${UBUNTU_CODENAME}/ llvm-toolchain-${UBUNTU_CODENAME}-${LLVM_VERSION} main" \
        > "/etc/apt/sources.list.d/llvm-${LLVM_VERSION}.list" \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        "clang-${LLVM_VERSION}" \
        "clang-tools-${LLVM_VERSION}" \
    && update-alternatives --install /usr/bin/clang clang "/usr/bin/clang-${LLVM_VERSION}" 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ "/usr/bin/clang++-${LLVM_VERSION}" 100 \
    && rm -rf /var/lib/apt/lists/*

ENV CC=clang \
    CXX=clang++ \
    PATH="/root/.local/bin:${PATH}"

RUN pipx install conan

WORKDIR /src
COPY . .

ENV RELEASE_PLATFORM=linux
RUN case "${TARGETARCH}" in \
        amd64) export RELEASE_ARCH=x86_64 ;; \
        arm64) export RELEASE_ARCH=aarch64 ;; \
        *) echo "unsupported TARGETARCH=${TARGETARCH}" >&2; exit 1 ;; \
    esac \
    && chmod +x scripts/ci/release-build.sh scripts/ci/release-conan-profile.sh \
    && ./scripts/ci/release-build.sh

FROM ubuntu:${UBUNTU_VERSION} AS runtime

ARG DEBIAN_FRONTEND=noninteractive
ARG BEEZ_VERSION=1.0.2
ARG BUILD_DATE

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        libtbb12 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --uid 10001 beez

COPY --from=build /src/build/build/Release/bin/beez /usr/local/bin/beez

LABEL org.opencontainers.image.title="Beez" \
      org.opencontainers.image.description="Build and task orchestrator with Lua DSL" \
      org.opencontainers.image.version="${BEEZ_VERSION}" \
      org.opencontainers.image.created="${BUILD_DATE}" \
      org.opencontainers.image.source="https://github.com/Coditary/Beez"

USER beez
WORKDIR /work

ENTRYPOINT ["/usr/local/bin/beez"]
CMD ["--help"]
