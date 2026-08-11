#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

assert_exit() {
    local expected="$1"
    shift
    set +e
    "$@" >/dev/null 2>&1
    local status=$?
    set -e
    if [[ "${status}" -ne "${expected}" ]]; then
        echo "Expected exit ${expected}, got ${status} for: $*" >&2
        exit 1
    fi
}

assert_output_contains() {
    local needle="$1"
    shift
    local output
    set +e
    output="$("$@" 2>&1)"
    set -e
    if [[ "${output}" != *"${needle}"* ]]; then
        echo "Expected output to contain '${needle}'." >&2
        echo "Command: $*" >&2
        echo "Output: ${output}" >&2
        exit 1
    fi
}

test_conan_graph_converter_rejects_invalid_json() {
    local graph="${TMP_DIR}/invalid-graph.json"
    echo '{"not":"a graph"}' >"${graph}"
    assert_exit 1 python3 "${ROOT_DIR}/scripts/conan-graph-to-cyclonedx.py" "${graph}" "${TMP_DIR}/out.json"
    assert_output_contains "invalid Conan graph" \
        python3 "${ROOT_DIR}/scripts/conan-graph-to-cyclonedx.py" "${graph}" "${TMP_DIR}/out.json"
}

test_conan_graph_converter_rejects_missing_file() {
    assert_exit 1 python3 "${ROOT_DIR}/scripts/conan-graph-to-cyclonedx.py" \
        "${TMP_DIR}/missing.json" "${TMP_DIR}/out.json"
    assert_output_contains "failed to read Conan graph" \
        python3 "${ROOT_DIR}/scripts/conan-graph-to-cyclonedx.py" \
        "${TMP_DIR}/missing.json" "${TMP_DIR}/out.json"
}

test_ci_install_rejects_invalid_version() {
  assert_exit 1 env OSV_SCANNER_VERSION='1.0;rm -rf /' \
      bash "${ROOT_DIR}/scripts/ci-install-osv-scanner.sh"
  assert_output_contains "invalid OSV_SCANNER_VERSION" \
      env OSV_SCANNER_VERSION='not-a-version' \
      bash "${ROOT_DIR}/scripts/ci-install-osv-scanner.sh"
}

test_ci_install_cppcheck_rejects_invalid_version() {
    assert_exit 1 env CPPCHECK_VERSION='2.21;rm -rf /' \
        bash "${ROOT_DIR}/scripts/ci-install-cppcheck.sh"
    assert_output_contains "invalid CPPCHECK_VERSION" \
        env CPPCHECK_VERSION='not-a-version' \
        bash "${ROOT_DIR}/scripts/ci-install-cppcheck.sh"
}

test_dependency_audit_missing_scanner() {
    local home="${TMP_DIR}/home-without-scanner"
    mkdir -p "${home}"
    assert_exit 2 env HOME="${home}" PATH="/usr/bin:/bin" OSV_SCANNER_AUTO_INSTALL=0 \
        bash "${ROOT_DIR}/scripts/dependency-audit.sh" build "${TMP_DIR}/report-missing-scanner"
    assert_output_contains "osv-scanner not found" \
        env HOME="${home}" PATH="/usr/bin:/bin" OSV_SCANNER_AUTO_INSTALL=0 \
        bash "${ROOT_DIR}/scripts/dependency-audit.sh" build "${TMP_DIR}/report-missing-scanner"
}

test_dependency_audit_prefers_pinned_scanner_over_path_hijack() {
    local home="${TMP_DIR}/home-pinned"
    local fake_bin="${TMP_DIR}/fake-path/bin"
    local reports="${TMP_DIR}/report-path-hijack"
    local sbom_dir="${reports}/sbom"
    mkdir -p "${home}/.local/bin" "${fake_bin}" "${sbom_dir}"

    cat >"${home}/.local/bin/osv-scanner" <<'EOF'
#!/usr/bin/env bash
if [[ "$1" == "--version" ]]; then
    echo "pinned osv-scanner version: 9.9.9"
    exit 0
fi
echo "pinned scanner used"
exit 0
EOF
    chmod +x "${home}/.local/bin/osv-scanner"

    cat >"${fake_bin}/osv-scanner" <<'EOF'
#!/usr/bin/env bash
echo "PATH HIJACK"
exit 0
EOF
    chmod +x "${fake_bin}/osv-scanner"

    cat >"${sbom_dir}/cyclonedx.json" <<'EOF'
{
  "bomFormat": "CycloneDX",
  "specVersion": "1.4",
  "components": [
    {
      "type": "library",
      "name": "testpkg",
      "version": "1.0.0",
      "purl": "pkg:conan/testpkg@1.0.0"
    }
  ]
}
EOF

    cat >"${reports}/conan.lock" <<'EOF'
{
  "version": "0.5",
  "requires": ["testpkg/1.0.0"]
}
EOF

    local output
    output="$(env HOME="${home}" PATH="${fake_bin}:/usr/bin:/bin" \
        DEPENDENCY_AUDIT_SBOM="${sbom_dir}/cyclonedx.json" \
        DEPENDENCY_AUDIT_LOCKFILE="${reports}/conan.lock" \
        bash "${ROOT_DIR}/scripts/dependency-audit.sh" build "${reports}" 2>&1)"
    if [[ "${output}" == *"PATH HIJACK"* ]]; then
        echo "dependency-audit used PATH scanner instead of pinned ~/.local/bin install" >&2
        exit 1
    fi
    if [[ "${output}" != *"pinned scanner used"* ]]; then
        echo "Expected pinned scanner output, got: ${output}" >&2
        exit 1
    fi
}

test_dependency_audit_malformed_lockfile_fails() {
    local home="${TMP_DIR}/home-malformed"
    local reports="${TMP_DIR}/report-malformed"
    local lockfile="${reports}/conan.lock"
    local sbom="${reports}/sbom/cyclonedx.json"
    mkdir -p "${home}/.local/bin" "$(dirname "${sbom}")"

    cat >"${home}/.local/bin/osv-scanner" <<'EOF'
#!/usr/bin/env bash
if [[ "$1" == "--version" ]]; then
    echo "fake osv-scanner version: 0.0.0"
    exit 0
fi
echo "not valid lockfile"
exit 128
EOF
    chmod +x "${home}/.local/bin/osv-scanner"
    echo 'not json' >"${lockfile}"
    echo '{"bomFormat":"CycloneDX","specVersion":"1.4","components":[]}' >"${sbom}"

    assert_exit 128 env HOME="${home}" PATH="/usr/bin:/bin" \
        DEPENDENCY_AUDIT_SBOM="${sbom}" \
        DEPENDENCY_AUDIT_LOCKFILE="${lockfile}" \
        bash "${ROOT_DIR}/scripts/dependency-audit.sh" build "${reports}"
}

test_dependency_audit_missing_sbom_override() {
    local home="${TMP_DIR}/home-missing-sbom"
    mkdir -p "${home}/.local/bin"
    cat >"${home}/.local/bin/osv-scanner" <<'EOF'
#!/usr/bin/env bash
exit 0
EOF
    chmod +x "${home}/.local/bin/osv-scanner"

    assert_exit 2 env HOME="${home}" PATH="/usr/bin:/bin" \
        DEPENDENCY_AUDIT_SBOM="${TMP_DIR}/does-not-exist.json" \
        bash "${ROOT_DIR}/scripts/dependency-audit.sh" build "${TMP_DIR}/report-missing-sbom"
}

test_security_script_requires_compile_commands() {
    assert_exit 2 bash "${ROOT_DIR}/scripts/security.sh" "${TMP_DIR}/missing-build"
    assert_output_contains "compile_commands.json not found" \
        bash "${ROOT_DIR}/scripts/security.sh" "${TMP_DIR}/missing-build"
}

test_conan_graph_converter_rejects_invalid_json
test_conan_graph_converter_rejects_missing_file
test_ci_install_rejects_invalid_version
test_ci_install_cppcheck_rejects_invalid_version
test_dependency_audit_missing_scanner
test_dependency_audit_prefers_pinned_scanner_over_path_hijack
test_dependency_audit_malformed_lockfile_fails
test_dependency_audit_missing_sbom_override
test_security_script_requires_compile_commands

echo "security script adversarial tests passed"
