#!/usr/bin/env bash
set -euo pipefail

INSTALL_DIR="${BEEZ_INSTALL_DIR:-"$HOME/.local/bin"}"
TARGET="${INSTALL_DIR}/beez"

CONFIG_DIR="${XDG_CONFIG_HOME:-"$HOME/.config"}/beez"
BASH_COMPLETION_DIR="${BEEZ_BASH_COMPLETION_DIR:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/bash-completion/completions"}"
ZSH_COMPLETION_DIR="${BEEZ_ZSH_COMPLETION_DIR:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/zsh/site-functions"}"
BASH_COMPLETION_TARGET="${BASH_COMPLETION_DIR}/beez"
ZSH_COMPLETION_TARGET="${ZSH_COMPLETION_DIR}/_beez"

HOOK_BEGIN="# >>> beez shell completion >>>"
HOOK_END="# <<< beez shell completion <<<"

removed=0

remove_shell_hook() {
    local rc_file="$1"

    if [[ ! -f "$rc_file" ]]; then
        return 0
    fi

    if ! grep -qF "$HOOK_BEGIN" "$rc_file"; then
        return 0
    fi

    local tmp_file
    tmp_file="$(mktemp)"
    awk -v begin="$HOOK_BEGIN" -v end="$HOOK_END" '
        $0 == begin { skip = 1; next }
        $0 == end { skip = 0; next }
        skip == 0 { print }
    ' "$rc_file" >"$tmp_file"
    mv "$tmp_file" "$rc_file"
    echo "Removed shell hook from ${rc_file}"
    removed=1
}

if [[ -L "$TARGET" ]]; then
    rm "$TARGET"
    echo "Removed ${TARGET}"
    removed=1
elif [[ -e "$TARGET" ]]; then
    echo "Refusing to remove non-symlink: ${TARGET}" >&2
    exit 1
fi

for completion_target in "$BASH_COMPLETION_TARGET" "$ZSH_COMPLETION_TARGET"; do
    if [[ -L "$completion_target" ]]; then
        rm "$completion_target"
        echo "Removed ${completion_target}"
        removed=1
    elif [[ -e "$completion_target" ]]; then
        echo "Refusing to remove non-symlink: ${completion_target}" >&2
        exit 1
    fi
done

for config_target in \
    "${CONFIG_DIR}/activate.zsh" \
    "${CONFIG_DIR}/activate.bash" \
    "${CONFIG_DIR}/config.env"; do
    if [[ -L "$config_target" || -f "$config_target" ]]; then
        rm -f "$config_target"
        echo "Removed ${config_target}"
        removed=1
    fi
done

remove_shell_hook "$HOME/.zshrc"
remove_shell_hook "$HOME/.bashrc"

if [[ "$removed" -eq 0 ]]; then
    echo "beez is not installed at ${TARGET}"
fi
