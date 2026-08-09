# Contributing to Beez

Thank you for your interest in Beez. This file is the entry point for contributors. Detailed guides live in the [wiki](https://github.com/Coditary/Beez/wiki) and in [`docs/`](docs/).

## Quick links

| Topic | Where |
|-------|--------|
| Build and install | [Wiki: Building and Setup](https://github.com/Coditary/Beez/wiki/Building-and-Setup) |
| Repository layout | [Wiki: Repository Layout](https://github.com/Coditary/Beez/wiki/Repository-Layout) |
| Tests | [Wiki: Testing](https://github.com/Coditary/Beez/wiki/Testing) |
| Code quality | [Wiki: Code Quality](https://github.com/Coditary/Beez/wiki/Code-Quality) |
| Vertical feature workflow | [`docs/vertical-feature.md`](docs/vertical-feature.md) |
| Changelog | [`CHANGELOG.md`](CHANGELOG.md) |

## Getting started

### Requirements

- CMake >= 3.24
- Clang >= 17 (CI uses LLVM 22)
- Conan 2.x
- Ninja

Optional: ccache or sccache, clang-format, clang-tidy, cmake-format, cppcheck

### First-time setup

```bash
git clone https://github.com/Coditary/Beez.git
cd Beez
make setup CONAN_PROFILE=conan/profiles/clang-release
make build
make test
./build/build/Release/bin/beez --help
```

Set compilers if needed:

```bash
export CC=clang
export CXX=clang++
```

## How we develop features

Beez uses **vertical** development: a user-visible change should span the layers it touches (tests, core, plugins, CLI), not a single module in isolation.

Typical flow:

1. Clarify the user story and acceptance criteria
2. Write failing tests (unit, then integration, then system where appropriate)
3. Implement the minimum to pass
4. Refactor
5. Run `make all` before opening a pull request

See [`docs/vertical-feature.md`](docs/vertical-feature.md) and the [Wiki: Feature Development Workflow](https://github.com/Coditary/Beez/wiki/Feature-Development-Workflow).

## Before you open a pull request

1. Branch from `main`
2. Keep changes focused (no unrelated drive-by refactors)
3. Register new source and test files in the relevant `CMakeLists.txt`
4. Run the full local quality pipeline:

```bash
make all
```

Optional: reproduce CI locally:

```bash
STRICT_CI=1 ./scripts/ci.sh
```

For faster iteration during development:

```bash
make test              # run tests
make format            # apply formatting
make lint-stale        # incremental lint
make help              # list all Make targets
```

## Pull request guidelines

Open pull requests against `main` on [Coditary/Beez](https://github.com/Coditary/Beez).

A good PR includes:

- **What** changed and **why** (behavior or bug fixed)
- **How** it was tested (new or updated tests, manual steps if needed)
- Notes on user-visible changes (wiki update appreciated for larger features)

Commit messages should describe intent, not only file names. There is no enforced commit message format.

### Definition of done

A change is ready to merge when:

- All CI checks pass
- `make all` passes locally (or discrepancies are explained in the PR)
- Tests cover new behavior, including failure paths where relevant
- DSL or parser changes include fuzz corpus updates when appropriate (`tests/fuzz/corpus/lua_dsl/`)

## Continuous integration

GitHub Actions workflow **Quality Assurance** (`.github/workflows/ci.yml`) runs on pushes and pull requests to `main`:

| Step | Make target |
|------|-------------|
| Build | `make build` |
| Tests | `make test` |
| Format | `make format-check` |
| Lint | `make lint` |
| Static analysis | `make analyze` |
| Security | `make security` |
| Coverage | `make coverage` |
| Sanitizers | `make sanitize` |
| Fuzzer | `make fuzzer-smoke` |

Environment: Ubuntu 24.04, Clang (LLVM 22), Conan 2, Ninja.

A separate **CodeQL** workflow (`.github/workflows/codeql.yml`) analyzes C++ on pushes to `main` and `develop`, on pull requests targeting `main`, and on a weekly schedule.

## Code style

- C++20, extensions off (`CMAKE_CXX_EXTENSIONS OFF`)
- **clang-format** for `*.cpp`, `*.hpp`, `*.h` under `src/`, `include/`, `tests/`
- **cmake-format** for listed `CMakeLists.txt` files
- **clang-tidy** and **cppcheck** via `make lint`, `make analyze`, and `make security`
- Match existing naming, module boundaries, and patterns in touched files

Configuration: `.clang-format` at the repo root; cmake-format via `cmakelang`.

## Reporting issues

Use [GitHub Issues](https://github.com/Coditary/Beez/issues). For bugs, include:

- Beez version (`beez --version`) or commit hash
- OS and compiler version
- Minimal `build.lua` or steps to reproduce
- Expected vs actual behavior
- Relevant output (`beez --verbose`, run log path under `.cache/logs/`)

For security vulnerabilities, use [GitHub Security Advisories](https://github.com/Coditary/Beez/security/advisories/new) or contact the maintainers privately. Do not file public issues for security bugs.

## Documentation

- **User and contributor docs:** [Beez Wiki](https://github.com/Coditary/Beez/wiki)
- **In-repo developer docs:** [`docs/`](docs/)
- **Release notes:** [`CHANGELOG.md`](CHANGELOG.md)

User-facing behavior changes should be reflected in the wiki when practical.

## License

By contributing, you agree that your contributions are licensed under the [Apache License 2.0](LICENSE), the same license as the project.
