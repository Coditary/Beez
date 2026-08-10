# In-repository documentation

Developer-focused docs live here. User guides and reference material live in the [Beez Wiki](https://github.com/Coditary/Beez/wiki).

## Documents

| File | Purpose |
|------|---------|
| [`vertical-feature.md`](vertical-feature.md) | End-to-end feature workflow: TDD, QA, wiki updates, coverage target |
| [`worker-api.md`](worker-api.md) | Lua step context: `ctx:spawn`, `ctx:wait`, `ctx:wait_all` |
| [`net-api.md`](net-api.md) | HTTP/REST and downloads: `beez.net.*` |
| [`archive-api.md`](archive-api.md) | Archives: `beez.archive.*` |
| [`../CONTRIBUTING.md`](../CONTRIBUTING.md) | How to contribute, PR expectations, CI |
| [`../CHANGELOG.md`](../CHANGELOG.md) | Release notes |
| [`../README.md`](../README.md) | Project overview and quick start |

## Wiki (separate repository)

The wiki is maintained in the [Beez wiki repository](https://github.com/Coditary/Beez/wiki) (clone the wiki git repo from GitHub). User-visible behavior must be reflected there when features ship.

Common pages to update:

| Change type | Wiki pages |
|-------------|------------|
| CLI flags or behavior | [CLI Flag Reference](https://github.com/Coditary/Beez/wiki/CLI-Flag-Reference), [CLI Overview](https://github.com/Coditary/Beez/wiki/CLI-Overview) |
| Config keys | [Config Reference](https://github.com/Coditary/Beez/wiki/Config-Reference), [Configuration Overview](https://github.com/Coditary/Beez/wiki/Configuration-Overview) |
| DSL / `build.lua` | [DSL Overview](https://github.com/Coditary/Beez/wiki/DSL-Overview), related declaration pages, [`worker-api.md`](worker-api.md) for workers |
| Caching | [Caching](https://github.com/Coditary/Beez/wiki/Caching) chapter |
| Terminal UI | [UI and Output](https://github.com/Coditary/Beez/wiki/UI-and-Output) chapter |
| Build / contributor setup | [Building and Setup](https://github.com/Coditary/Beez/wiki/Building-and-Setup), [Feature Development Workflow](https://github.com/Coditary/Beez/wiki/Feature-Development-Workflow) |

See [`vertical-feature.md`](vertical-feature.md) for when and how to update documentation as part of a feature.

## Keeping docs current

When you change Beez:

1. **User-visible behavior** (CLI, DSL, config, cache, UI): update the wiki in the same PR cycle when possible, or immediately after merge
2. **Releases**: add an entry to [`CHANGELOG.md`](../CHANGELOG.md)
3. **Setup or install paths**: update [`README.md`](../README.md) and wiki Building and Setup
4. **Contributor workflow**: update this folder, [`CONTRIBUTING.md`](../CONTRIBUTING.md), and wiki Development chapter together
5. **QA expectations** (coverage, `make all`): update [`vertical-feature.md`](vertical-feature.md), wiki Code Quality, and CI docs in sync

Do not leave the wiki outdated while in-repo docs describe new behavior.

## Quality targets

| Target | Where enforced |
|--------|----------------|
| `make all` green | CI (`build-test`, `coverage`, `sanitize`, `fuzzer` jobs), [`CONTRIBUTING.md`](../CONTRIBUTING.md) |
| Line coverage **≥ 85%** on `src/` | `make coverage`, `scripts/coverage-report.sh`, CI `coverage` job |
| Fuzz smoke pass | `make fuzzer-smoke`, CI `fuzzer` job |

Run `make coverage` locally and open `report/coverage/index.html` before opening a PR that adds or changes production code.
