# Beez bash completion activation — sourced from ~/.bashrc.

if [[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/beez/config.env" ]]; then
    # shellcheck source=/dev/null
    source "${XDG_CONFIG_HOME:-$HOME/.config}/beez/config.env"
fi

if [[ -n "${BEEZ_COMPLETION_DIR:-}" && -f "${BEEZ_COMPLETION_DIR}/beez.bash" ]]; then
    # shellcheck source=/dev/null
    source "${BEEZ_COMPLETION_DIR}/beez.bash"
fi
