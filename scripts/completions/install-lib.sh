#!/usr/bin/env bash
# Shared helpers for installing Beez shell completion files and hooks.

beez_data_dir() {
    BEEZ_DATA_DIR="${BEEZ_DATA_DIR:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/beez"}"
}

beez_completion_paths() {
    CONFIG_DIR="${XDG_CONFIG_HOME:-"$HOME/.config"}/beez"
    USER_COMPLETION_DIR="${CONFIG_DIR}/completions"
    BASH_COMPLETION_DIR="${BEEZ_BASH_COMPLETION_DIR:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/bash-completion/completions"}"
    ZSH_COMPLETION_DIR="${BEEZ_ZSH_COMPLETION_DIR:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/zsh/site-functions"}"
    BASH_COMPLETION_TARGET="${BASH_COMPLETION_DIR}/beez"
    ZSH_COMPLETION_TARGET="${ZSH_COMPLETION_DIR}/_beez"
}

stage_beez_completion_bundle() {
    local completions_src="$1"

    beez_data_dir
    local dest="${BEEZ_DATA_DIR}/completions"
    if [[ -d "$dest" ]] && [[ "$(cd "$completions_src" && pwd)" == "$(cd "$dest" && pwd)" ]]; then
        return 0
    fi

    mkdir -p "$dest"

    cp -f "${completions_src}/beez.bash" "${dest}/beez.bash"
    cp -f "${completions_src}/beez.zsh" "${dest}/beez.zsh"
    cp -f "${completions_src}/activate.bash" "${dest}/activate.bash"
    cp -f "${completions_src}/activate.zsh" "${dest}/activate.zsh"
    cp -f "${completions_src}/install-lib.sh" "${dest}/install-lib.sh"
}

install_beez_completion_files() {
    local completions_src="$1"

    beez_completion_paths
    mkdir -p "$CONFIG_DIR" "$USER_COMPLETION_DIR" "$BASH_COMPLETION_DIR" "$ZSH_COMPLETION_DIR"

    cp -f "${completions_src}/beez.bash" "${USER_COMPLETION_DIR}/beez.bash"
    cp -f "${completions_src}/beez.zsh" "${USER_COMPLETION_DIR}/beez.zsh"
    cp -f "${completions_src}/activate.bash" "${USER_COMPLETION_DIR}/activate.bash"
    cp -f "${completions_src}/activate.zsh" "${USER_COMPLETION_DIR}/activate.zsh"
    cp -f "${completions_src}/install-lib.sh" "${USER_COMPLETION_DIR}/install-lib.sh"

    cat >"${CONFIG_DIR}/config.env" <<EOF
BEEZ_COMPLETION_DIR="${USER_COMPLETION_DIR}"
EOF

    ln -sf "${USER_COMPLETION_DIR}/activate.zsh" "${CONFIG_DIR}/activate.zsh"
    ln -sf "${USER_COMPLETION_DIR}/activate.bash" "${CONFIG_DIR}/activate.bash"
    ln -sf "${USER_COMPLETION_DIR}/beez.bash" "$BASH_COMPLETION_TARGET"
    ln -sf "${USER_COMPLETION_DIR}/beez.zsh" "$ZSH_COMPLETION_TARGET"

    stage_beez_completion_bundle "$completions_src"
}

install_beez_completion_hooks() {
    local hook_begin="# >>> beez shell completion >>>"
    local hook_end="# <<< beez shell completion <<<"
    local zsh_hook='[[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/beez/activate.zsh" ]] && source "${XDG_CONFIG_HOME:-$HOME/.config}/beez/activate.zsh"'
    local bash_hook='[[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/beez/activate.bash" ]] && source "${XDG_CONFIG_HOME:-$HOME/.config}/beez/activate.bash"'

    _install_beez_shell_hook "$HOME/.zshrc" "$hook_begin" "$hook_end" "$zsh_hook"
    _install_beez_shell_hook "$HOME/.bashrc" "$hook_begin" "$hook_end" "$bash_hook"
}

_install_beez_shell_hook() {
    local rc_file="$1"
    local hook_begin="$2"
    local hook_end="$3"
    local hook_line="$4"

    if [[ ! -f "$rc_file" ]]; then
        return 0
    fi

    if grep -qF "$hook_begin" "$rc_file"; then
        echo "Shell hook already present in ${rc_file}"
        return 0
    fi

    {
        echo ""
        echo "$hook_begin"
        echo "$hook_line"
        echo "$hook_end"
    } >>"$rc_file"

    echo "Added shell hook to ${rc_file}"
}

print_beez_completion_paths() {
    beez_completion_paths
    echo "  config: ${CONFIG_DIR}/config.env"
    echo "  user:   ${USER_COMPLETION_DIR}"
    echo "  bash:   ${BASH_COMPLETION_TARGET}"
    echo "  zsh:    ${ZSH_COMPLETION_TARGET}"
}
