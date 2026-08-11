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

_beez_core_flags() {
    printf '%s\n' \
        --help --version --verbose --dry-run --no-cache --show-config --config-options \
        --clean-cache --update --install --threads --list --phase --step --install-completion \
        -h -v -j -p -s
}

_beez_config_option_completions() {
    local prefix="$1"
    local beez_cmd

    beez_cmd="$(_beez_cmd)"
    if [[ -z "$beez_cmd" ]]; then
        return 0
    fi

    "$beez_cmd" --complete-config-options "$prefix" 2>/dev/null
}

_beez_first_argument_suggestions() {
    local cur="$1"
    local root

    _beez_core_flags
    if _beez_find_root >/dev/null; then
        root="$(_beez_find_root)"
        _beez_targets "$root"
    fi
}

_beez() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD - 1]}"
    local root
    local index

    for ((index = 1; index < COMP_CWORD; ++index)); do
        if [[ "${COMP_WORDS[index]}" == "--config-options" ]]; then
            mapfile -t COMPREPLY < <(compgen -W "$(_beez_config_option_completions "$cur")" -- "$cur")
            return 0
        fi
    done

    if [[ "$cur" == -* ]]; then
        mapfile -t COMPREPLY < <(compgen -W "$(_beez_core_flags)" -- "$cur")
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

    if [[ $COMP_CWORD -eq 1 ]]; then
        mapfile -t COMPREPLY < <(compgen -W "$(_beez_first_argument_suggestions "$cur")" -- "$cur")
    fi
}

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    complete -F _beez beez
fi
