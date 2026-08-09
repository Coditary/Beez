# Changelog

All notable changes to Beez are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0] - 2026-08-09

Milestone 11: Quality of Life ([PR #11](https://github.com/Coditary/Beez/pull/11))

### Added

#### CLI

- Shell tab completion for bash and zsh (`--dump-completion`, `--install-completion`, `make install-beez-completion`)
- `--show-config` to print the merged active configuration and each setting's origin
- `--config-options [PATH]` to list config keys or allowed enum values for a dotted path
- `--complete-config-options` for shell completion of config paths
- `--silent` to suppress run output and rely on the exit code only
- `--error` to show errors only (no progress or success summary on successful runs)
- `--update` to migrate cache files to the active compression settings without changing cached content
- "Did you mean?" suggestions when a task, workflow, or step name is misspelled

#### Configuration

- Global user settings via `~/.config/beez/config.lua` (see `scripts/config.lua.example`)
- Project settings through `beez.config({ ... })` in `build.lua`
- Config schema introspection for all settings keys and enum values
- Expanded environment support: dotenv loading, default vars, hash vars, secret masking
- Performance settings: cache write strategy, filesystem metadata cache, mmap hashing thresholds, GC tuning, thread pinning

#### Cache

- Configurable hash algorithms and seed for cache fingerprints
- Configurable cache compression (algorithm, level, mode)
- Heuristic to pick a better cache write strategy and reduce computation
- Improved auto-compression saving strategy for cache storage

#### Terminal UI and output

- Terminal UI configuration: colors, truecolor, themes, icons, progress bars, spinners
- Run summary modes: `minimal`, `simple`, `compact`, `data`
- Table-style formatting for listings and summaries
- Terminal-width-aware wrapping for worker error output
- Failed worker output is shown in non-verbose modes when a step fails
- Run log and worker log files under `.cache/logs/` (`--log-file`, `--no-log-file`)

### Changed

- Root `build.lua` uses real parallel workflow steps for maintainer pipelines
- CI pipeline uses LLVM 22 (via `scripts/ci-install-llvm.sh`)

### Fixed

- Shell completion bug
- Failing tests after milestone changes
- Linter and static analyzer issues from UI and config work
- Coverage pipeline now fails when coverage tests fail

### [0.1.0] - prior releases

Earlier milestones (multithreading, incremental caching, workers, input/output caching, and earlier CLI and DSL work) shipped before this changelog was introduced. See git history and [PR #10](https://github.com/Coditary/Beez/pull/10) and earlier pull requests for details.
