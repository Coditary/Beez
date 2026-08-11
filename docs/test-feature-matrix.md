# Test Feature Matrix (Phase 1 Inventory)

As of 2026-08-09. Basis for the comprehensive test suite (phases 2–7).
Legend: **✓** covered · **~** partial · **✗** missing · **U/I/S/F** = Unit / Integration / System / Fuzz

---

## 1. CLI flags

| Flag | Unit | Integration | System | Fuzz | Status | Existing tests / gap |
|------|------|-------------|--------|------|--------|----------------------|
| `-h, --help` | ✓ | ✓ | ✓ | — | ✓ | `test_cli_app`, `test_cli`, `test_errors` |
| `-v, --version` | ✓ | ✓ | — | — | ~ | Parse + subprocess; no system scenario |
| `--verbose` | ✓ | ✓ | — | — | ~ | `test_cli` VerboseMode; no system |
| `--silent` | ✓ | ✗ | ✗ | — | ~ | Parse + `test_settings` CliOverrides; **no runtime subprocess** |
| `--error` | ✓ | ✗ | ✗ | — | ~ | Parse + `test_settings`; **no runtime subprocess** |
| `--dry-run` | ✓ | ✓ | — | — | ~ | `test_cli` DryRun; orchestrator unit; no system |
| `--no-cache` | ✓ | ~ | ✗ | — | ~ | Parse + orchestrator in-code (`enableCache=false`); **no CLI `--no-cache` subprocess** |
| `--show-config` | ✓ | ✗ | ✗ | — | ~ | Parse only in `test_cli_app`; **no output test** |
| `--config-options` | ✓ | ~ | — | — | ~ | Parse + `test_config_schema` + bash-completion script; **no CLI output test** |
| `--complete-config-options` | ✓ | ~ | — | — | ~ | Parse + bash-completion script |
| `--dump-completion` | ✓ | ~ | — | — | ~ | Parse + script content check; **no runtime dump test** |
| `--clean-cache` | ✓ | ✓ | — | — | ~ | `test_cli` CleanCache; no system |
| `--update` | ✓ | ✗ | ✗ | — | ~ | Parse only; **no cache-update runtime test** |
| `--install-completion` | ✓ | ✓ | — | — | ~ | `test_install_completion`, standalone script |
| `-j, --threads` | ✓ | ✗ | ✗ | — | ~ | Parse + reject-zero; **no effect test** |
| `--list tasks` | ✓ | ✓ | — | — | ~ | `test_cli` ListTasks |
| `--list workflows` | ✗ | ✗ | ✗ | — | ✗ | **Missing entirely** |
| `--list steps` | ✗ | ✗ | ✗ | — | ✗ | **Missing entirely** |
| `--list phases` | ✓ | ✗ | ✗ | — | ~ | Parse only in `test_cli_app` |
| `-p, --phase` | ✓ | ✓ | ✓ | — | ✓ | Colon + bracket syntax in integration + system |
| `-s, --step` | ~ | ✓ | ✓ | — | ✓ | Integration + system; no dedicated unit parse test |
| `--log-file` | ✗ | ✗ | ✗ | — | ✗ | **Missing entirely** (parse + runtime) |
| `--no-log-file` | ✗ | ✗ | ✗ | — | ✗ | **Missing entirely** |
| `--` user options | ✓ | ✗ | ✗ | — | ~ | Parse only in `test_cli_app` |
| Flag conflicts (`--silent --verbose`) | ✓ | ✗ | — | — | ~ | Unit RejectsConflictingOutputFlags |

**CLI summary:** 8 flags without runtime integration test · 3 flags missing entirely (`--log-file`, `--no-log-file`, `--list workflows/steps`)

---

## 2. `beez.config()` / config schema

### 2.1 `performance`

| Key | Unit | Integration | System | Status | Notes |
|-----|------|-------------|--------|--------|-------|
| `max_threads` | ✓ | ✗ | ✗ | ~ | `test_lua_settings`, `test_settings`; no runtime |
| `cache_write_strategy` | ✓ | ✗ | ✗ | ~ | `test_performance_options`, schema completions |
| `cache_fs_metadata` | ~ | ✗ | ✗ | ~ | Schema completions only |
| `use_mmap_for_hashing` | ✗ | ✗ | ✗ | ✗ | Defined in schema, **no test** |
| `mmap_hashing_min_bytes` | ✓ | ✗ | ✗ | ~ | `test_performance_options` NormalizesMmapThreshold |
| `optimize_gc_for_throughput` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `pin_threads_to_cores` | ✗ | ✗ | ✗ | ✗ | **Missing** |

### 2.2 `cache`

| Key | Unit | Integration | System | Status | Notes |
|-----|------|-------------|--------|--------|-------|
| `path` | ✓ | ~ | ✗ | ~ | `test_settings`, `test_lua_settings`; no system fixture |
| `enabled` | ✓ | ~ | ✗ | ~ | Unit + pipeline in-code disable |
| `protect` | ✓ | ✗ | ✗ | ~ | `test_lua_settings` |
| `hash.algorithm` | ✓ | ~ | ✗ | ~ | `test_cache_options`, pipeline |
| `hash.seed` | ✓ | ✗ | ✗ | ~ | `test_cache_options` |
| `compress.algorithm` | ✓ | ✗ | ✗ | ~ | `test_cache_options` |
| `compress.level` | ✓ | ✗ | ✗ | ~ | `test_cache_options` |
| `compress.mode` | ✓ | ✗ | ✗ | ~ | `test_cache_options` |

### 2.3 `ui`

| Key | Unit | Integration | System | Status | Notes |
|-----|------|-------------|--------|--------|-------|
| `output_mode` | ✓ | ~ | ✗ | ~ | `test_lua_settings`, `test_ui_options` |
| `colors` | ✓ | ✗ | ✗ | ~ | `test_ui_options` |
| `truecolor` | ✓ | ✗ | ✗ | ~ | `test_ui_options` |
| `theme` / `themes` | ✓ | ✗ | ✗ | ~ | `test_ui_options` (+ RejectsUnknownTheme) |
| `icons` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `animation.progress` | ✓ | ✗ | ✗ | ~ | `test_ui_options`, `test_lua_settings` |
| `animation.indicator` | ✓ | ✗ | ✗ | ~ | `test_ui_options` |
| `animation.indicator_spin_interval` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `log_level` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `hide_cache_hits` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `prefix` / `prefix_format` | ✓ | ~ | ✗ | ~ | `test_ui_options` + integration clean-mode worker test |
| `show_time_saved` | ✓ | ✗ | ✗ | ~ | `test_ui_options` FormatsFullyCachedRunEnd |
| `summary` | ✓ | ✗ | ✗ | ~ | `test_ui_options` |
| `logging.run_log` | ~ | ✗ | ✗ | ~ | `test_logging_settings` (run_log_writer) |
| `logging.run_log_file` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `logging.log_steps` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `logging.workers` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `logging.workers_dir` | ✗ | ✗ | ✗ | ✗ | **Missing** |

### 2.4 `env`

| Key | Unit | Integration | System | Status | Notes |
|-----|------|-------------|--------|--------|-------|
| `load_dotenv` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `dotenv_overrides_system` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `files` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `vars` | ✓ | ✗ | ✗ | ~ | `test_lua_settings`, `test_settings` ApplyEnvironment |
| `hash_vars` | ~ | ✗ | ✗ | ~ | `test_env_settings` ResolveUsesDefaultsForHashLists |
| `ignore_vars_for_hashing` | ✗ | ✗ | ✗ | ✗ | **Missing** |
| `mask_secrets` | ✗ | ✗ | ✗ | ✗ | **Missing** |

### 2.5 Config merge and CLI override

| Scenario | Unit | Integration | System | Status |
|----------|------|-------------|--------|--------|
| Global → project (`beez.config`) merge | ✓ | ✗ | ✗ | ~ |
| CLI overrides project (`--no-cache`, `--verbose`) | ✓ | ✗ | ✗ | ~ |
| `--show-config` shows merged config | ✗ | ✗ | ✗ | ✗ |
| `build.lua` config affects cache path | ✓ | ~ | ✗ | ~ |
| `build.lua` config affects UI output | ✗ | ~ | ✗ | ~ |

**Config summary:** ~25 keys without a direct test · no system fixture for config-heavy projects

---

## 3. DSL elements (`build.lua`)

| Element | Unit | Integration | System | Fuzz | Status | Notes |
|---------|------|-------------|--------|------|--------|-------|
| `task(name, cmd)` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `task(name, {cmds})` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `task(name, {step refs})` | ✓ | ✓ | ✓ | — | ✓ | |
| `step({name, phase, scope, run})` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `step({description})` | ✓ | ✓ | — | — | ~ | |
| `step({run = function})` | ✓ | ✓ | — | — | ~ | spawn/wait; see [`worker-api.md`](worker-api.md) |
| `configure_step` before/after registration | ✓ | ✓ | — | — | ✓ | |
| Step inline config | ✓ | ✓ | — | — | ✓ | |
| `artifacts` / `inputs` / `outputs` | ✓ | ✓ | — | ✓ | ✓ | Cache pipelines |
| `mutate` | ✓ | — | — | — | ~ | Unit only |
| `order` declaration | ✓ | — | — | — | ~ | Unit only |
| `workflow(name, sequential)` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `workflow(name, {parallel})` | ✗ | ✗ | ✗ | ✗ | ✗ | **Removed** |
| `beez.config({...})` | ✓ | ~ | ✗ | ~ | ~ | Unit + 1 integration case |
| `beez.env(name)` | ✓ | ✗ | ✗ | ✓ | ~ | Unit + fuzz seed |
| Phase-task shorthand `{phase, scope, run}` | ~ | ✗ | ✗ | ✓ | ~ | Negative in unit; fuzz seed only |
| `depends_on` (task) | ✗ | ✗ | ✗ | — | ✗ | **Not implemented** (docs example) |
| Duplicate name (task/step) | ✗ | ✗ | ✗ | — | ✗ | **Missing** |
| Empty task table | ✗ | ✗ | ✗ | — | ✗ | **Missing** |
| Step without `name`/`phase`/`run` | ~ | ✗ | ✗ | — | ~ | Partial negative (missing run) |
| Invalid `run` type | ✗ | ✗ | ✗ | — | ✗ | **Missing** |
| Syntax error | ✓ | ✓ | ✓ | ✓ | ✓ | |
| Orphan task (no workflow) | ✓ | ✓ | ✓ | ✓ | ✓ | |

---

## 4. Runtime / orchestration

| Behavior | Unit | Integration | System | Status |
|----------|------|-------------|--------|--------|
| Task execution (shell) | ✓ | ✓ | ✓ | ✓ |
| Step-by-name (`-s`) | ✓ | ✓ | ✓ | ✓ |
| Phase execution (`-p`) | ✓ | ✓ | ✓ | ✓ |
| Workflow execution | ✓ | ✓ | ✓ | ✓ |
| Parallel phases in workflow | ✗ | ✗ | ✗ | **Removed** |
| Worker spawn/wait | ✓ | ✓ | — | ✓ ([`worker-api.md`](worker-api.md)) |
| `ctx:wait` / `ctx:wait_all` result tables | ✓ | ✓ | — | ✓ |
| Step cache hit/miss | ✓ | ✓ | ✗ | ~ |
| Success cache hit/miss | ✓ | ✓ | ✗ | ~ |
| Cache disabled → no skip | ✓ | ✓ | ✗ | ~ |
| Dry-run skips execution | ✓ | ✓ | — | ~ |
| Artifact dependency ordering | ✓ | — | — | ~ |
| Parallel independent steps | ✓ | — | — | ~ |
| Unknown name → error + DidYouMean | ✓ | ✓ | ✓ | ✓ |
| Missing build.lua | ✓ | ✓ | ✓ | ✓ |
| Task failure → non-zero exit | ✓ | ✓ | ✓ | ✓ |
| Elapsed duration reporting | — | ✓ | — | ~ |

---

## 5. Source modules vs. unit tests

| `src/` module | Unit test file | Status |
|---------------|----------------|--------|
| `core/registry/` | `core/registry/test_registry.cpp` | ✓ |
| `core/orchestrator/` | `core/orchestrator/test_orchestrator.cpp` | ✓ |
| `core/config/settings.cpp` | `core/config/test_settings.cpp` | ✓ |
| `core/config/config_schema.cpp` | `core/config/test_config_schema.cpp` | ✓ |
| `core/cache/` | `core/cache/test_cache_*` | ✓ |
| `core/config/ui_options.cpp` | `core/config/test_ui_options.cpp` | ✓ |
| `core/config/performance_options.cpp` | `core/config/test_performance_options.cpp` | ~ |
| `core/env/env_file.cpp` | `core/env/test_env_file.cpp` | ✓ |
| `core/config/env_settings.cpp` | `test_env_settings` (in settings) | ~ |
| `core/config/config_paths.cpp` | `core/config/test_config_paths.cpp` | ✓ |
| `core/model/step_config.cpp` | indirect via DSL/cache | ~ |
| `plugins/host/plugin_host.cpp` | `plugin/host/test_plugin_host.cpp` | ✓ |
| `core/execution/concurrency/worker_pool.cpp` | `core/execution/concurrency/test_worker_pool.cpp` | ✓ |
| `core/execution/concurrency/thread_pool.cpp` | `core/execution/concurrency/test_thread_pool.cpp` | ✓ |
| `plugins/lua/lua_dsl.cpp` | `test_lua_dsl.cpp` | ✓ |
| `plugins/lua/lua_settings.cpp` | `test_lua_settings.cpp` | ✓ |
| `plugins/lua/lua_step_config.cpp` | indirect | ~ |
| `plugins/shell/shell_executor.cpp` | `test_shell_executor.cpp` | ✓ |
| `cli/cli_app.cpp` | `test_cli_app.cpp` | ✓ |
| `cli/run_target.cpp` | `test_run_target.cpp` | ~ |
| `cli/install_completion.cpp` | integration only | ~ |
| `cli/list_formatter.cpp` | `test_list_formatter.cpp` | ✓ |
| `cli/name_suggestion.cpp` | `test_name_suggestion.cpp` | ✓ |
| `logging/run_log_writer.cpp` | `test_logging_settings.cpp` | ~ |
| `logging/recording_logger.cpp` | — | ✗ |
| `logging/progress_spinner.cpp` | indirect via ui_options | ~ |
| `logging/spdlog_backend.cpp` | indirect via logger | ~ |
| `logging/output_mode.cpp` | `test_output_mode.cpp` | ✓ |
| `logging/worker_output_format.cpp` | `test_worker_output_format.cpp` | ✓ |
| `app/main.cpp` | via integration CLI | ~ |

---

## 6. System fixtures

| Fixture | Path | Covered scenarios | Status |
|---------|------|-------------------|--------|
| `minimal` | `fixtures/minimal/` | hello/fail/clean tasks | ✓ |
| `workflows` | `fixtures/workflows/` | build/ci workflow, phases, steps | ✓ |
| `phase-tasks` | `fixtures/phase-tasks/` | Phase/step invocation | ✓ |
| `invalid-syntax` | `fixtures/invalid-syntax/` | Parse error | ✓ |
| `empty` | `fixtures/empty/` | Missing build.lua | ✓ |
| `config-cache` | — | Custom cache path/algo | ✗ planned |
| `config-ui` | — | UI output_mode/theme | ✗ planned |
| `config-env` | — | env.vars in steps | ✗ planned |
| `cache-behavior` | — | Hit/miss/re-run | ✗ planned |
| `flag-matrix` | — | CLI flag combinations | ✗ planned |

---

## 7. Fuzzer

| Area | Corpus seeds | Dictionary | Status |
|------|--------------|------------|--------|
| `task()` forms | `step.lua`, `task_with_step.lua`, `orphan_task.lua` | ✓ | ✓ |
| `workflow()` forms | `workflow_sequential.lua` | ✓ | ✓ |
| `step()` forms | `step.lua`, `step_config.lua`, `step_artifact_fields.lua` | ✓ | ✓ |
| `beez.env()` | `beez_env.lua` | — | ~ |
| `beez.config()` | — | — | ✗ |
| Invalid / mixed | `invalid_syntax.lua`, `mixed.lua` | — | ~ |
| Phase-task shorthand | `phase_task.lua` | — | ~ |
| Worker spawn | `worker_spawn.lua` | — | ~ |
| Nested config tables | — | — | ✗ planned |
| Invalid config types | — | — | ✗ planned |
| Missing required fields | — | — | ✗ planned |
| Large/stress tables | — | — | ✗ planned |
| Dictionary keywords | 12 entries | small | ~ |

**Fuzz summary:** 12 seeds · no config fuzzing · dictionary incomplete

---

## 8. Prioritized gaps (input for phases 2–5)

### P0 — Runtime flags without integration test

1. `--silent`, `--error` (output behavior)
2. `--show-config` (merged config output)
3. `--no-cache` via CLI subprocess (not only in-code)
4. `--update` (cache storage update)
5. `--log-file`, `--no-log-file`
6. `--list workflows`, `--list steps`
7. `-j/--threads` (effect)

### P1 — Config from `build.lua` without system test

1. Custom `cache.path` → files land there
2. `ui.output_mode` → output format
3. `env.vars` → step sees variable
4. CLI override vs. project config

### P2 — DSL negative / edge cases

1. Duplicate names
2. Invalid types in config/step
3. Empty tables
4. `depends_on` (when implemented: red test first)

### P3 — Untested modules

1. `config_paths.cpp`
2. `recording_logger.cpp`
3. Extended `lua_step_config` tests

### P4 — Fuzzer

1. ≥10 new corpus seeds (config, invalid, stress)
2. Expand dictionary (config keys, DSL keywords)

---

## 9. Planned test files (new)

| File | Level | Priority |
|------|-------|----------|
| `tests/integration/app/test_cli_flags.cpp` | Integration | P0 |
| `tests/integration/app/test_config_runtime.cpp` | Integration | P1 |
| `tests/integration/core/test_cache_flag_pipeline.cpp` | Integration | P0 |
| `tests/system/scenarios/test_cli_flags.cpp` | System | P0 |
| `tests/system/scenarios/test_config_build_lua.cpp` | System | P1 |
| `tests/system/scenarios/test_cache_usecases.cpp` | System | P1 |
| `tests/system/scenarios/test_list_entities.cpp` | System | P0 |
| `tests/unit/core/config/test_config_paths.cpp` | Unit | P3 |
| `tests/unit/logging/test_recording_logger.cpp` | Unit | P3 |
| `tests/system/fixtures/config-cache/` | Fixture | P1 |
| `tests/system/fixtures/config-ui/` | Fixture | P1 |
| `tests/system/fixtures/config-env/` | Fixture | P1 |
| `tests/system/fixtures/cache-behavior/` | Fixture | P1 |
| `tests/system/fixtures/flag-matrix/` | Fixture | P0 |
| `tests/fuzz/corpus/lua_dsl/config_*.lua` (≥5) | Fuzz | P4 |

---

## 10. Metrics (baseline)

| Metric | Value |
|--------|-------|
| Unit test files | 41 |
| Integration test files | 8 (+ 3 shell scripts) |
| System scenario files | 4 |
| System fixtures | 5 |
| Fuzz corpus seeds | 12 |
| CLI flags total | 22 |
| CLI flags with runtime integration test | ~10 |
| Config schema keys (estimated) | ~45 |
| Config keys with unit test | ~20 |
| DSL elements | 18 |
| `src/` modules without a direct unit test | 4 (`config_paths`, `recording_logger`, `progress_spinner`, `spdlog_backend`) |

---

*Update after each test phase. See also [`vertical-feature.md`](vertical-feature.md) and the [Beez wiki Testing page](https://github.com/Coditary/Beez/wiki/Testing).*
