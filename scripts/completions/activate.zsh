# Beez zsh completion activation — sourced from ~/.zshrc after compinit.

if [[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/beez/config.env" ]]; then
    # shellcheck source=/dev/null
    source "${XDG_CONFIG_HOME:-$HOME/.config}/beez/config.env"
fi

if [[ -n ${BEEZ_COMPLETION_DIR:-} && -f "${BEEZ_COMPLETION_DIR}/beez.zsh" ]]; then
    # shellcheck source=/dev/null
    source "${BEEZ_COMPLETION_DIR}/beez.zsh"
    compdef _beez beez
fi
