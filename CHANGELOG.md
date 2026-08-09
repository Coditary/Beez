# Changelog

All notable changes to Beez are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Milestone 13: Fixing Bugs, Security and Testability

### Added

#### CI and quality assurance

- Parallel GitHub Actions CI pipeline (format, static-check, build+test, coverage, sanitizer, fuzzer, ThreadSanitizer, SBOM/dependency audit) with aggregate `ci-success` gate
- Reusable `.github/actions/setup-beez` composite action (LLVM, Conan, ccache, optional cppcheck/gcovr/osv-scanner/cmakelang)
- Docs-only change detection: build and test jobs are skipped when only `docs/` or `*.md` files change
- `make tidy-ci` — single combined clang-tidy pass for CI (`scripts/tidy-ci.sh`)
- `make static-check` — standalone cppcheck pass without a full build (`scripts/static-check.sh`)
- `make tsan` / `setup-tsan` — ThreadSanitizer configure, build, and test run
- `make robustness` — fast subset of crash/edge-case system tests
- `make dependency-audit` — OSV vulnerability scan of Conan dependencies
- `make sbom` — CycloneDX SBOM generation from the Conan dependency graph
- `make fuzzer-seed-verify`, `make fuzzer-torture`, `make fuzz-seeds` — additional fuzz workflows
- Coverage enforcement: 85% minimum line coverage on `src/` (`MIN_LINE_COVERAGE`), HTML report, JSON badge, and GitHub check
- Auto-updated coverage badge on `main` (`.github/badges/coverage.json`)
- `scripts/ci-install-osv-scanner.sh` — pinned OSV scanner install for CI and local use
- `scripts/conan-graph-to-cyclonedx.py` — Conan graph → CycloneDX SBOM converter
- `scripts/coverage-badge-json.py`, `scripts/coverage-github-summary.sh` — coverage reporting for CI
- `scripts/fuzz-common.sh`, `scripts/fuzz-seed-verify.sh`, `scripts/fuzz-torture.sh`, `scripts/generate-fuzz-seeds.sh`
- `build.lua` QA steps: `qa:dependency-audit`, `beez tsan` workflow target

#### Security

- Dependency vulnerability scanning via `osv-scanner` against a CycloneDX SBOM (`scripts/dependency-audit.sh`)
- SBOM submitted to the GitHub Dependency Graph in CI
- Adversarial integration tests for tooling (`tests/integration/scripts/test_security_scripts.sh`): malformed SBOMs, PATH hijacking, invalid install versions, missing tooling
- Hardened `ci-install-osv-scanner.sh` against invalid version strings (command injection)
- `make security` now also runs the dependency audit

#### Tests

- **Test feature matrix** (`docs/test-feature-matrix.md`) — Phase 1 inventory of CLI flags, config keys, DSL fields, and coverage gaps
- **System tests:** robustness (`test_robustness.cpp`), negative fixtures (`test_negative_fixtures.cpp`), cache adversarial (`test_cache_adversarial.cpp`), DSL field matrix (`test_dsl_field_matrix.cpp`), CLI flags (`test_cli_flags.cpp`), config via `build.lua` (`test_config_build_lua.cpp`), list entities (`test_list_entities.cpp`), cache use cases (`test_cache_usecases.cpp`)
- **Integration tests:** CLI flag runtime (`test_cli_flags.cpp`), config runtime (`test_config_runtime.cpp`), cache flag pipeline (`test_cache_flag_pipeline.cpp`), parallel Lua steps (`test_parallel_lua_steps.cpp`), security scripts (`test_security_scripts.cpp`)
- **Unit tests:** config paths, content hash, orchestrator, step order, logger, logging settings, progress spinner, worker output format, Lua DSL fields, Lua settings (expanded)
- **Fuzz:** 89 corpus seeds (up from 12), field-matrix seeds, dictionary (`lua_dsl.dict`), expanded harness with config/env/order/registry stress inputs
- **Fixtures:** scratch project helper, cache-adversarial/config/negative/dsl/flag-matrix system fixtures

#### Core and DSL

- `Registry::validateConsistent()` — rejects `configure_step` calls for undefined steps
- `Registry::clear()` — reset registry state (used in tests)
- Duplicate task/step/workflow names now throw instead of silently overwriting
- `validateLoadedRegistry()` — post-load consistency checks for task step references
- Stricter Lua DSL validation: workflow phase/scope required fields, reject numeric/string workflow entries
- `isolateCallbackStepsInLevels()` — callback steps in the same parallel level are serialized to avoid shared-interpreter races

### Changed

- CI refactored from a single monolithic job to nine parallel jobs with shared setup action
- CodeQL workflow uses the shared `setup-beez` action
- `CONTRIBUTING.md`, `README.md`, `SECURITY.md`, and `docs/README.md` updated for the new QA pipeline
- `docs/vertical-feature.md` expanded with QA targets, coverage gate, and test matrix reference
- `scripts/ci.sh` documents parallel CI layout; runs format, static-check, build, test, tidy-ci, coverage, sanitize, fuzzer sequentially
- `make coverage` uses a dedicated coverage configure stamp and enforces the threshold via gcovr
- `build.lua` removes obsolete explicit `order()` chains now handled by the engine (callback serialization, mutate inference)
- Fuzz scripts refactored with shared `fuzz-common.sh`

### Fixed

- Segfault when multiple Lua callback steps shared a parallel execution level (order calculation now isolates callbacks)
- Step order errors are propagated correctly from `orderStepsInLevels` instead of being ignored
- Coverage pipeline validates that `.gcda` files are produced before reporting

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
