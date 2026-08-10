#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
REPORTS_DIR="${2:-report}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SECURITY_DIR="${ROOT_DIR}/${REPORTS_DIR}/security"
AUDIT_REPORT="${SECURITY_DIR}/dependency-audit.txt"
SBOM_JSON="${ROOT_DIR}/${REPORTS_DIR}/sbom/cyclonedx.json"

mkdir -p "${SECURITY_DIR}"

resolve_osv_scanner() {
    if [[ -n "${OSV_SCANNER:-}" && -x "${OSV_SCANNER}" ]]; then
        return 0
    fi

    if [[ -x "${HOME}/.local/bin/osv-scanner" ]]; then
        OSV_SCANNER="${HOME}/.local/bin/osv-scanner"
        return 0
    fi

    if command -v osv-scanner >/dev/null 2>&1; then
        OSV_SCANNER="osv-scanner"
        return 0
    fi

    if [[ "${OSV_SCANNER_AUTO_INSTALL:-0}" == "1" ]]; then
        "${ROOT_DIR}/scripts/ci-install-osv-scanner.sh"
        if [[ -x "${HOME}/.local/bin/osv-scanner" ]]; then
            OSV_SCANNER="${HOME}/.local/bin/osv-scanner"
            return 0
        fi
    fi

    return 1
}

if ! resolve_osv_scanner; then
    echo "error: osv-scanner not found in PATH" >&2
    echo "Install it with: ${ROOT_DIR}/scripts/ci-install-osv-scanner.sh" >&2
    echo "Or set OSV_SCANNER_AUTO_INSTALL=1 to bootstrap automatically." >&2
    exit 2
fi

echo "=== Generating Conan SBOM for dependency audit ==="
if [[ -n "${DEPENDENCY_AUDIT_SBOM:-}" ]]; then
    if [[ ! -f "${DEPENDENCY_AUDIT_SBOM}" ]]; then
        echo "error: DEPENDENCY_AUDIT_SBOM not found at ${DEPENDENCY_AUDIT_SBOM}" >&2
        exit 2
    fi
    SBOM_JSON="${DEPENDENCY_AUDIT_SBOM}"
else
    "${ROOT_DIR}/scripts/sbom-generate.sh" "${BUILD_DIR}" "${REPORTS_DIR}"
    SBOM_JSON="${ROOT_DIR}/${REPORTS_DIR}/sbom/cyclonedx.json"
fi

if [[ ! -f "${SBOM_JSON}" ]]; then
    echo "error: SBOM not found at ${SBOM_JSON}" >&2
    exit 2
fi

LOCKFILE="${DEPENDENCY_AUDIT_LOCKFILE:-${ROOT_DIR}/conan.lock}"
if [[ ! -f "${LOCKFILE}" ]] || [[ "${ROOT_DIR}/conanfile.py" -nt "${LOCKFILE}" ]]; then
    if [[ -z "${CONAN_PROFILE:-}" ]]; then
        CONAN_PROFILE="$("${ROOT_DIR}/scripts/ci-conan-profile.sh")"
    fi

    echo "=== Generating Conan lockfile for dependency audit ==="
    conan lock create "${ROOT_DIR}" \
        -pr "${CONAN_PROFILE}" \
        -pr:b "${CONAN_PROFILE}" \
        --lockfile-out="${LOCKFILE}"
fi

echo ""
echo "=== Dependency vulnerability scan (OSV database) ==="
echo "SBOM: ${SBOM_JSON}"
echo "Lockfile: ${LOCKFILE}"
echo "Scanner: ${OSV_SCANNER} $("${OSV_SCANNER}" --version 2>/dev/null | head -n1)"
echo ""

set +e
"${OSV_SCANNER}" scan --lockfile="${LOCKFILE}" --format=vertical --verbosity=warn 2>&1 | tee "${AUDIT_REPORT}"
SCAN_STATUS=$?
set -e

echo ""
if [[ "${SCAN_STATUS}" -eq 0 ]]; then
    echo "=== Dependency audit passed (no known vulnerabilities in OSV) ==="
    exit 0
fi

if [[ "${SCAN_STATUS}" -eq 1 ]]; then
    echo "=== Dependency audit failed (known vulnerabilities found) ===" >&2
    echo "See ${AUDIT_REPORT} for details." >&2
    exit 1
fi

echo "=== Dependency audit failed (osv-scanner exit ${SCAN_STATUS}) ===" >&2
exit "${SCAN_STATUS}"
