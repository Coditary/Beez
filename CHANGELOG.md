# Changelog

All notable changes to Beez are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

#### Network API (`beez.net`)

- `beez.net.get/post/put/delete/request` — REST with shared header table shape
- `beez.net.upload` — multipart file upload
- `beez.net.download` / `download_and_verify` — file download (+ optional hash check via `beez.crypto`)
- `beez.net.ping` / `is_online` — connectivity helpers
- `beez.net.set_proxy` — process-wide proxy URL
- [`docs/net-api.md`](docs/net-api.md)

#### Worker API (Lua step context)

- `ctx:wait(handle, options)` — optional second argument requests fields to return (`exitCode`, `output`, `duration`, `cached`, `name`, `id`, `dryRun`); omit options to wait without a return value
- `ctx:wait_all(handles, options)` — one shared options table for all workers; returns a 1-based array of per-worker result tables in handle order
- Worker shell output is captured and exposed through the `output` field after `wait` or `wait_all`
- Automatic worker names `{step-name}-{n}` (for example `compile-1`, `compile-2`); `name` is no longer required in `ctx:spawn`
- [`docs/worker-api.md`](docs/worker-api.md) — spawn, wait, and wait_all reference

### Changed

#### Worker API (breaking)

- `ctx:wait(handle)` no longer returns an exit code; use `ctx:wait(handle, { exitCode = true })` and read `result.exitCode`
- `ctx:wait_all(handles)` no longer returns an exit code; use `ctx:wait_all(handles, { exitCode = true })` or wait on individual handles
- `ctx:spawn` no longer requires `name`; an explicit `name` remains supported for logging overrides

---

Milestone 14: Codebase Refactoring

### Added

#### Project layout

- Domain-oriented directory structure: public headers under `include/beez/`, sources under `src/`, unit tests mirror the same layout under `tests/unit/`
- Core grouped into domains: `model/`, `registry/`, `config/`, `cache/`, `glob/`, `env/`, `runtime/`, `execution/`, `util/`, `orchestrator/`
- `include/beez/core/model/workflow_step.hpp` — extracted workflow step type
- `include/beez/core/util/temp_directory.hpp` — shared RAII temp-directory helper (tests and fuzz)

#### Orchestrator

- Split monolithic orchestrator into focused modules:
  - **Public API:** `orchestrator/orchestrator.hpp`, `errors.hpp`, `types.hpp`
  - **Internal access:** `orchestrator_access.hpp` (`orchestrator_detail::Access`)
  - **Run infrastructure** (`run/`): lifecycle, logged-run entry scope, cache flush/skip, shell/step execution, progress logging, stats, step counting, timing
  - **Execution paths** (`runners/`): task, step, phase, workflow, shell command, step callback

#### CLI

- Reorganized into `commands/` (`run`, `list`, `config`, `cache`), `parsing/` (`cli_parser`, `help_text`, `phase_argument`, `parsed_options`), `presentation/` (`entity_table`, `name_suggestion`), `completion/`
- `runner.hpp` and `session.hpp` for CLI execution flow

#### Logging

- Reorganized into `backends/` (recording, spdlog, null), `console/` (output mode, progress spinner, worker output format), `persistence/` (run log writer), `settings/`
- `spdlog_setup.cpp` extracted from the spdlog backend

#### Plugins

- Plugin host moved from `core/` to `src/plugins/host/` with dedicated `beez_plugin_host` CMake target
- Lua plugin split into `dsl/` (task/step/workflow parsers, registry validation, binder), `runtime/` (step config, worker parser), `settings/` (lua settings, settings overlay)

#### Config and cache

- Config UI helpers: `progress_format.hpp`, `resolve.hpp`, `run_summary.hpp`, `theme.hpp`; settings report under `config/report/`
- Cache step module: `filesystem_store`, `key_strategy`, `output_tracker`, `step_cache`, `types`
- Cache storage/fingerprint/success headers namespaced under `cache/storage/`, `cache/fingerprint/`, `cache/success/`

#### Tests

- Unit tests relocated to mirror `src/` (`tests/unit/core/config/…`, `tests/unit/plugin/…`, `tests/unit/logging/…`, etc.)
- New tests: `test_temp_directory.cpp`, `test_spdlog_logger.cpp`, `test_run_log_writer.cpp`; settings report tests under `config/report/`

### Changed

- All public API headers live under `include/beez/` (no headers co-located in `src/`)
- Include paths updated throughout the codebase (e.g. `beez/core/task.hpp` → `beez/core/model/task.hpp`, `beez/core/orchestrator.h` → `beez/core/orchestrator/orchestrator.hpp`)
- `orchestrator.cpp` slimmed to public entry points; execution logic lives in `orchestrator/run/` and `orchestrator/runners/`
- `phase_argument_parser` moved from core to `cli/parsing/phase_argument.hpp`
- `beez_plugin_lua` publicly links `beez_plugin_host` (correct static link order for fuzz and other consumers)
- Root `build.lua`: removed all commented `order()` declarations — the engine infers step ordering from phase+scope and callback isolation
- README and `docs/vertical-feature.md` updated with core domain layout table and orchestrator module map
- Fuzz corpus: parallel-workflow seeds updated to expect rejection (`field_workflow_parallel_rejected.lua`, etc.)

### Removed

- Legacy `include/beez/core/orchestrator.h` C-style header
- Workflow DSL `parallel` step groups — `workflow("name", { { parallel = { … } } })` is rejected at load time (`workflow step does not support 'parallel'`)
- Flat monolith source files at old `src/core/*.cpp` paths (orchestrator, step cache, plugin host, ui options, etc.)
- Obsolete fuzz corpus seeds for valid parallel workflows (`workflow_parallel.lua`, `workflow_nested_parallel.lua`, `deeply_nested_workflow.lua`, …)

### Fixed

- `beez all` quality pipeline: cppcheck, clang-tidy (include-cleaner, enum sizing, naming), and clang-format issues across refactored orchestrator modules
- Fuzzer build: undefined reference to `PluginHost::setDslLoader` when linking `fuzz_lua_dsl`

---

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
- Fuzz scripts refactored with shared `fuzz-common.sh`

### Fixed

- Tests, fuzzer, and shell-completion staging no longer create a stray `tmp/` folder in the project root when `TMPDIR` (or similar) is set to a relative path like `tmp`

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
