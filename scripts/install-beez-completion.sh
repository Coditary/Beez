#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ ! -f "${SCRIPT_DIR}/completions/install-lib.sh" ]]; then
    echo "Could not locate Beez completion files." >&2
    exit 1
fi

COMPLETIONS_SRC="${SCRIPT_DIR}/completions"

# shellcheck source=/dev/null
source "${COMPLETIONS_SRC}/install-lib.sh"

install_beez_completion_files "$COMPLETIONS_SRC"
install_beez_completion_hooks

if [[ "$(basename "$SCRIPT_DIR")" != "beez" ]]; then
    beez_data_dir
    mkdir -p "$BEEZ_DATA_DIR"
    cp -f "$0" "${BEEZ_DATA_DIR}/install-beez-completion.sh"
    chmod +x "${BEEZ_DATA_DIR}/install-beez-completion.sh"
fi

echo "Installed shell completion files:"
print_beez_completion_paths
echo ""
echo "Shell completion hook installed."
echo "Restart your shell or run: source ~/.zshrc"
echo "Then from a project with build.lua: beez <TAB>"
