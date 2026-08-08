# Beez bash completion — sourced automatically after `make install-beez`.
# Override the binary path for tests with BEEZ_COMPLETION_BEEZ.

_beez_cmd() {
    if [[ -n "${BEEZ_COMPLETION_BEEZ:-}" ]]; then
        printf '%s\n' "$BEEZ_COMPLETION_BEEZ"
        return 0
    fi

    command -v beez 2>/dev/null
}

_beez_find_root() {
    local dir="$PWD"
    while [[ "$dir" != "/" ]]; do
        if [[ -f "$dir/build.lua" ]]; then
            printf '%s\n' "$dir"
            return 0
        fi
        dir="$(dirname "$dir")"
    done
    return 1
}

_beez_list_names() {
    local kind="$1"
    local root="$2"
    local beez_cmd

    beez_cmd="$(_beez_cmd)"
    if [[ -z "$beez_cmd" ]]; then
        return 0
    fi

    (
        cd "$root" && "$beez_cmd" --list "$kind" 2>/dev/null
    ) | awk '/^-/{sep=1; next} sep && NF { print $1 }'
}

_beez_phase_scopes() {
    local root="$1"
    local beez_cmd

    beez_cmd="$(_beez_cmd)"
    if [[ -z "$beez_cmd" ]]; then
        return 0
    fi

    (
        cd "$root" && "$beez_cmd" --list steps 2>/dev/null
    ) | awk '/^-/{sep=1; next} sep && NF>=3 { print $2 ":" $3 }' | sort -u
}

_beez_targets() {
    local root="$1"
    _beez_list_names tasks "$root"
    _beez_list_names workflows "$root"
}

_beez() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD - 1]}"
    local root

    if [[ "$cur" == -* ]]; then
        COMPREPLY=($(compgen -W '--help --version --verbose --dry-run --no-cache --clean-cache --threads --list --phase --step -h -v -j -p -s' -- "$cur"))
        return 0
    fi

    case "$prev" in
        --list)
            COMPREPLY=($(compgen -W 'tasks workflows steps phases' -- "$cur"))
            return 0
            ;;
        -p | --phase)
            if _beez_find_root >/dev/null; then
                root="$(_beez_find_root)"
                COMPREPLY=($(compgen -W "$(_beez_phase_scopes "$root")" -- "$cur"))
            fi
            return 0
            ;;
        -s | --step)
            if _beez_find_root >/dev/null; then
                root="$(_beez_find_root)"
                COMPREPLY=($(compgen -W "$(_beez_list_names steps "$root")" -- "$cur"))
            fi
            return 0
            ;;
    esac

    if [[ $COMP_CWORD -eq 1 ]] && _beez_find_root >/dev/null; then
        root="$(_beez_find_root)"
        COMPREPLY=($(compgen -W "$(_beez_targets "$root")" -- "$cur"))
    fi
}

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    complete -F _beez beez
fi
