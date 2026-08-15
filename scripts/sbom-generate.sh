#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
REPORTS_DIR="${2:-report}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SBOM_DIR="${ROOT_DIR}/${REPORTS_DIR}/sbom"
GRAPH_JSON="${SBOM_DIR}/conan-graph.json"
SBOM_JSON="${SBOM_DIR}/cyclonedx.json"
CONVERTER="${ROOT_DIR}/plugins/coditary/conan/1.0.0/scripts/conan-graph-to-cyclonedx.py"

if [ -z "${CONAN_PROFILE:-}" ]; then
    CONAN_PROFILE="$("${ROOT_DIR}/scripts/ci-conan-profile.sh")"
fi

mkdir -p "${SBOM_DIR}"

cd "${ROOT_DIR}"
conan graph info . \
    -pr "${CONAN_PROFILE}" \
    -pr:b "${CONAN_PROFILE}" \
    --format=json \
    --out-file "${GRAPH_JSON}"

python3 "${CONVERTER}" "${GRAPH_JSON}" "${SBOM_JSON}"

echo "SBOM written to ${SBOM_JSON}"
