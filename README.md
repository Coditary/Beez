# Beez

Build and task orchestrator for software projects. Pipelines are defined in a Lua DSL (`build.lua`). Beez runs them with parallelism, incremental caching, and a terminal progress UI.

**Status:** pre-1.0 (API and DSL may change). See [`CHANGELOG.md`](CHANGELOG.md) for release notes.

[![Quality Assurance](https://github.com/Coditary/Beez/actions/workflows/ci.yml/badge.svg)](https://github.com/Coditary/Beez/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/Coditary/Beez/main/.github/badges/coverage.json)](https://github.com/Coditary/Beez/actions/workflows/ci.yml)

## Features

- **Lua DSL** (`build.lua`): steps, tasks, workflows, `order()`, per-step config, in-step **workers** (`ctx:spawn` / `ctx:wait`), **network** (`beez.net.*`), **archives** (`beez.archive.*`), **text** (`beez.text.*`)
- **Parallel execution** with worker threads (`-j`) for independent steps within a phase+scope
- **Caching**: step cache, success cache (Lua callbacks), configurable hash and compression
- **Phases and scopes** for filtering and organizing work (`beez -p compile:code`)
- **CLI**: list entities, dry-run, shell completion (bash/zsh)
- **Configuration**: global `~/.config/beez/config.lua`, project `beez.config()`, env/dotenv
- **Terminal UI**: output modes (`clean`, `verbose`, `errors`, `silent`), themes, run summaries, log files

## Documentation

| Topic | Link |
|-------|------|
| Wiki (main docs) | [github.com/Coditary/Beez/wiki](https://github.com/Coditary/Beez/wiki) |
| Quick reference | [Wiki: Quick Reference](https://github.com/Coditary/Beez/wiki/Quick-Reference) |
| CLI flags | [Wiki: CLI Flag Reference](https://github.com/Coditary/Beez/wiki/CLI-Flag-Reference) |
| Glossary | [Wiki: Glossary](https://github.com/Coditary/Beez/wiki/Glossary) |
| Contributor guide | [`CONTRIBUTING.md`](CONTRIBUTING.md) |
| Developer docs | [`docs/`](docs/) (incl. [`worker-api.md`](docs/worker-api.md), [`net-api.md`](docs/net-api.md), [`archive-api.md`](docs/archive-api.md), [`text-api.md`](docs/text-api.md)) |
| Vertical feature workflow | [`docs/vertical-feature.md`](docs/vertical-feature.md) |

## Requirements

- CMake >= 3.24
- Clang >= 17 (CI uses LLVM 22)
- Conan 2.x
- Ninja

Optional: ccache or sccache, clang-format, clang-tidy, cmake-format, cppcheck (CI pins 2.21.1)

## Quick start

```bash
git clone https://github.com/Coditary/Beez.git
cd Beez
make setup CONAN_PROFILE=conan/profiles/clang-release
make build
./build/build/Release/bin/beez --help
```

Install locally (optional):

```bash
make install-beez              # symlink to ~/.local/bin/beez
make install-beez-completion  # tab completion for bash/zsh
```

### Minimal `build.lua`

```lua
step({
    name = "hello",
    phase = "demo",
    scope = "default",
    run = "echo hello",
})

task("hi", "echo hi")

workflow("all", {
    { phase = "demo", scope = "default" },
})
```

```bash
beez -s hello    # run step "hello"
beez hi          # run task "hi"
beez all         # run workflow "all"
```

Copy [`scripts/config.lua.example`](scripts/config.lua.example) to `~/.config/beez/config.lua` for user defaults.

## Development

```bash
make help          # list all Make targets
make test          # run tests
make robustness    # crash/edge-case system tests only (faster than make test)
make format        # apply clang-format + cmake-format
make lint          # clang-tidy + cmake-format check
make analyze       # static analysis
make security      # code security checks + OSV dependency audit
make dependency-audit  # Conan SBOM vulnerability scan only
make coverage      # coverage report (fails below 85% line coverage on src/)
make sbom          # CycloneDX SBOM from Conan dependencies
make all           # full QA pipeline (same as CI)
```

Reproduce CI locally:

```bash
STRICT_CI=1 ./scripts/ci.sh
```

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for pull request expectations, code style, and the full CI matrix.

CI also generates a CycloneDX SBOM from Conan dependencies and submits it to GitHub's **Dependency Graph** (Repository → **Insights** → **Dependency graph**). Run `make sbom` locally to produce `report/sbom/cyclonedx.json`.

Debug build with sanitizers:

```bash
make sanitize      # ASan + UBSan debug build + tests
```

## Project layout

| Path | Purpose |
|------|---------|
| `src/core/` | Core domains: `model/`, `registry/`, `config/`, `cache/`, `glob/`, `env/`, `runtime/`, `execution/`, `orchestrator/` |
| `src/plugins/lua/` | Lua DSL loader |
| `src/plugins/shell/` | Shell command execution |
| `src/cli/` | CLI parsing and completion |
| `src/app/` | `main` entry point |
| `tests/unit/` | Unit tests |
| `tests/integration/` | Integration tests |
| `tests/system/` | Black-box CLI and pipeline tests |
| `tests/fuzz/` | Lua DSL fuzzer |
| `build.lua` | Maintainer dogfooding pipeline (`beez build`, `beez quality`, …) |

## Security

Report vulnerabilities privately. See [`SECURITY.md`](SECURITY.md).

## License

[Apache License 2.0](LICENSE)
