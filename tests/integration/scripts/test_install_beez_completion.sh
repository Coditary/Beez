#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TMP_HOME="$(mktemp -d)"
trap 'rm -rf "$TMP_HOME"' EXIT

export HOME="$TMP_HOME"
export XDG_CONFIG_HOME="${HOME}/.config"
export XDG_DATA_HOME="${HOME}/.local/share"
touch "${HOME}/.zshrc"
touch "${HOME}/.bashrc"

"${ROOT_DIR}/scripts/install-beez-completion.sh"

CONFIG_DIR="${XDG_CONFIG_HOME}/beez"
USER_COMPLETION_DIR="${CONFIG_DIR}/completions"
BASH_TARGET="${XDG_DATA_HOME}/bash-completion/completions/beez"
ZSH_TARGET="${XDG_DATA_HOME}/zsh/site-functions/_beez"
DATA_SCRIPT="${XDG_DATA_HOME}/beez/install-beez-completion.sh"

[[ -f "${CONFIG_DIR}/config.env" ]]
[[ -f "${USER_COMPLETION_DIR}/beez.bash" ]]
[[ -f "${USER_COMPLETION_DIR}/beez.zsh" ]]
[[ ! -L "${USER_COMPLETION_DIR}/beez.bash" ]]
[[ -L "${CONFIG_DIR}/activate.zsh" ]]
[[ -L "$BASH_TARGET" ]]
[[ -L "$ZSH_TARGET" ]]

grep -qF "BEEZ_COMPLETION_DIR=\"${USER_COMPLETION_DIR}\"" "${CONFIG_DIR}/config.env"
grep -qF "# >>> beez shell completion >>>" "${HOME}/.zshrc"

# Binary-only path: run installer from the staged data bundle.
"${DATA_SCRIPT}"
grep -qF "# >>> beez shell completion >>>" "${HOME}/.bashrc"

echo "install-beez-completion tests passed"
