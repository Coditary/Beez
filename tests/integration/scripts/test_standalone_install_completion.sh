#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${BEEZ_EXECUTABLE:-}" ]]; then
    echo "BEEZ_EXECUTABLE is required" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

STANDALONE_BIN="${TMP_DIR}/beez"
cp "$BEEZ_EXECUTABLE" "$STANDALONE_BIN"

export HOME="${TMP_DIR}/home"
export XDG_CONFIG_HOME="${HOME}/.config"
export XDG_DATA_HOME="${HOME}/.local/share"
mkdir -p "$HOME"
touch "${HOME}/.zshrc"

"$STANDALONE_BIN" --install-completion

CONFIG_ENV="${XDG_CONFIG_HOME}/beez/config.env"
USER_COMPLETION="${XDG_CONFIG_HOME}/beez/completions/beez.zsh"

[[ -f "$CONFIG_ENV" ]]
[[ -f "$USER_COMPLETION" ]]
grep -qF "# >>> beez shell completion >>>" "${HOME}/.zshrc"

echo "standalone binary install-completion tests passed"
