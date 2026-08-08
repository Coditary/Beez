#compdef beez
# Beez zsh completion — loaded via scripts/completions/activate.zsh

_beez_cmd() {
    if [[ -n ${BEEZ_COMPLETION_BEEZ:-} ]]; then
        print -r -- "$BEEZ_COMPLETION_BEEZ"
        return 0
    fi

    command -v beez 2>/dev/null
}

_beez_find_root() {
    local dir="$PWD"
    while [[ $dir != / ]]; do
        if [[ -f $dir/build.lua ]]; then
            print -r -- "$dir"
            return 0
        fi
        dir=${dir:h}
    done
    return 1
}

_beez_list_names() {
    local kind=$1
    local root=$2
    local beez_cmd

    beez_cmd=$(_beez_cmd)
    if [[ -z $beez_cmd ]]; then
        return 0
    fi

    cd "$root" && "$beez_cmd" --list "$kind" 2>/dev/null | awk '/^-/{sep=1; next} sep && NF { print $1 }'
}

_beez_phase_scopes() {
    local root=$1
    local beez_cmd

    beez_cmd=$(_beez_cmd)
    if [[ -z $beez_cmd ]]; then
        return 0
    fi

    cd "$root" && "$beez_cmd" --list steps 2>/dev/null \
        | awk '/^-/{sep=1; next} sep && NF>=3 { print $2 ":" $3 }' \
        | sort -u
}

_beez_targets() {
    local root=$1
    _beez_list_names tasks "$root"
    _beez_list_names workflows "$root"
}

_beez() {
    local context state line
    local root
    local -a items

    _arguments -C \
        '(-h --help)'{-h,--help}'[Display help]' \
        '(-v --version)'{-v,--version}'[Display version]' \
        '--verbose[Enable verbose logging]' \
        '--dry-run[Build the graph without executing]' \
        '--no-cache[Disable caching]' \
        '--clean-cache[Remove .cache/ before running]' \
        '--update[Apply cache storage updates for the active configuration]' \
        '(-j --threads)-j[Maximum worker threads]' \
        '(-j --threads)--threads[Maximum worker threads]:threads:' \
        '--list[List registered entities]:kind:(tasks workflows steps phases)' \
        '(-p --phase)-p[Run a phase]:phase:->phase' \
        '(-p --phase)--phase[Run a phase]:phase:->phase' \
        '(-s --step)-s[Run a single step]:step:->step' \
        '(-s --step)--step[Run a single step]:step:->step' \
        '1:target:->target' && return 0

    case $state in
        target)
            if root=$(_beez_find_root); then
                items=("${(@f)$(_beez_targets "$root")}")
                _describe -t targets 'beez targets' items
            fi
            ;;
        phase)
            if root=$(_beez_find_root); then
                items=("${(@f)$(_beez_phase_scopes "$root")}")
                _describe -t phases 'beez phases' items
            fi
            ;;
        step)
            if root=$(_beez_find_root); then
                items=("${(@f)$(_beez_list_names steps "$root")}")
                _describe -t steps 'beez steps' items
            fi
            ;;
    esac
}
