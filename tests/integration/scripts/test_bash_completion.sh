#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${BEEZ_EXECUTABLE:-}" ]]; then
    echo "BEEZ_EXECUTABLE is required" >&2
    exit 1
fi

if [[ ! -x "$BEEZ_EXECUTABLE" ]]; then
    echo "BEEZ_EXECUTABLE is not executable: ${BEEZ_EXECUTABLE}" >&2
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
COMPLETION_SCRIPT="${ROOT_DIR}/scripts/completions/beez.bash"

if [[ ! -f "$COMPLETION_SCRIPT" ]]; then
    echo "Completion script not found: ${COMPLETION_SCRIPT}" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

cat >"${TMP_DIR}/build.lua" <<'EOF'
task("alpha", "true")
task("beta", "true")
workflow("build", {})
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "true",
})
EOF

export BEEZ_COMPLETION_BEEZ="$BEEZ_EXECUTABLE"
# shellcheck source=/dev/null
source "$COMPLETION_SCRIPT"

run_completion() {
    local -a words=("$@")
    local index=$(( ${#words[@]} - 1 ))
    COMP_WORDS=("${words[@]}")
    COMP_CWORD=$index
    COMP_LINE="${words[*]}"
    COMP_POINT=${#COMP_LINE}
    cur="${words[$index]}"
    COMPREPLY=()
    _beez
    printf '%s\n' "${COMPREPLY[@]}"
}

assert_contains() {
    local needle="$1"
    shift
    local item
    for item in "$@"; do
        if [[ "$item" == "$needle" ]]; then
            return 0
        fi
    done
    echo "Expected completion to contain '${needle}', got: $*" >&2
    exit 1
}

assert_not_contains() {
    local needle="$1"
    shift
    local item
    for item in "$@"; do
        if [[ "$item" == "$needle" ]]; then
            echo "Expected completion to not contain '${needle}', got: $*" >&2
            exit 1
        fi
    done
}

(
    cd "$TMP_DIR"
    mapfile -t target_suggestions < <(run_completion beez al)
    assert_contains alpha "${target_suggestions[@]}"
    assert_not_contains beta "${target_suggestions[@]}"

    mapfile -t list_suggestions < <(run_completion beez --list ta)
    assert_contains tasks "${list_suggestions[@]}"

    mapfile -t phase_suggestions < <(run_completion beez -p gen)
    assert_contains "generate:code" "${phase_suggestions[@]}"

    mapfile -t step_suggestions < <(run_completion beez -s gen)
    assert_contains gen-code "${step_suggestions[@]}"

    mapfile -t config_root_suggestions < <(run_completion beez --config-options "")
    assert_contains performance "${config_root_suggestions[@]}"
    assert_contains cache "${config_root_suggestions[@]}"

    mapfile -t config_nested_suggestions < <(run_completion beez --config-options performance.cache)
    assert_contains performance.cache_write_strategy "${config_nested_suggestions[@]}"
    assert_contains performance.cache_fs_metadata "${config_nested_suggestions[@]}"

    mapfile -t config_object_suggestions < <(run_completion beez --config-options performance)
    assert_contains performance.cache_write_strategy "${config_object_suggestions[@]}"
    assert_contains performance.max_threads "${config_object_suggestions[@]}"

    mapfile -t flag_suggestions < <(run_completion beez --conf)
    assert_contains --config-options "${flag_suggestions[@]}"

    mapfile -t first_arg_suggestions < <(run_completion beez "")
    assert_contains --config-options "${first_arg_suggestions[@]}"
)

echo "bash completion tests passed"
