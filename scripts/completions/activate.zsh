# Beez zsh completion activation — sourced from ~/.zshrc after compinit.
# Loads the completion script embedded in the installed beez binary so tab
# completion stays in sync after upgrades without re-running --install-completion.

if [[ -f ${XDG_CONFIG_HOME:-$HOME/.config}/beez/config.env ]]; then
    # shellcheck source=/dev/null
    source "${XDG_CONFIG_HOME:-$HOME/.config}/beez/config.env"
fi

if command -v beez >/dev/null 2>&1; then
    # shellcheck source=/dev/null
    source <(beez --dump-completion zsh 2>/dev/null)
elif [[ -n ${BEEZ_COMPLETION_DIR:-} && -f ${BEEZ_COMPLETION_DIR}/beez.zsh ]]; then
    # shellcheck source=/dev/null
    source "${BEEZ_COMPLETION_DIR}/beez.zsh"
    compdef _beez beez
fi
