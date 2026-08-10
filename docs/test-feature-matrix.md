# Test Feature Matrix (Phase 1 Inventar)

Stand: 2026-08-09. Basis für die umfängliche Test-Suite (Phasen 2–7).
Legende: **✓** abgedeckt · **~** teilweise · **✗** fehlt · **U/I/S/F** = Unit / Integration / System / Fuzz

---

## 1. CLI-Flags

| Flag | Unit | Integration | System | Fuzz | Status | Bestehende Tests / Lücke |
|------|------|-------------|--------|------|--------|--------------------------|
| `-h, --help` | ✓ | ✓ | ✓ | — | ✓ | `test_cli_app`, `test_cli`, `test_errors` |
| `-v, --version` | ✓ | ✓ | — | — | ~ | Parse + subprocess; kein System-Szenario |
| `--verbose` | ✓ | ✓ | — | — | ~ | `test_cli` VerboseMode; kein System |
| `--silent` | ✓ | ✗ | ✗ | — | ~ | Parse + `test_settings` CliOverrides; **kein Runtime-Subprocess** |
| `--error` | ✓ | ✗ | ✗ | — | ~ | Parse + `test_settings`; **kein Runtime-Subprocess** |
| `--dry-run` | ✓ | ✓ | — | — | ~ | `test_cli` DryRun; Orchestrator unit; kein System |
| `--no-cache` | ✓ | ~ | ✗ | — | ~ | Parse + Orchestrator in-code (`enableCache=false`); **kein CLI `--no-cache` subprocess** |
| `--show-config` | ✓ | ✗ | ✗ | — | ~ | Nur Parse in `test_cli_app`; **kein Output-Test** |
| `--config-options` | ✓ | ~ | — | — | ~ | Parse + `test_config_schema` + bash-completion script; **kein CLI-Output-Test** |
| `--complete-config-options` | ✓ | ~ | — | — | ~ | Parse + bash-completion script |
| `--dump-completion` | ✓ | ~ | — | — | ~ | Parse + Script-Content-Check; **kein Runtime-Dump-Test** |
| `--clean-cache` | ✓ | ✓ | — | — | ~ | `test_cli` CleanCache; kein System |
| `--update` | ✓ | ✗ | ✗ | — | ~ | Nur Parse; **kein Cache-Update-Runtime-Test** |
| `--install-completion` | ✓ | ✓ | — | — | ~ | `test_install_completion`, standalone script |
| `-j, --threads` | ✓ | ✗ | ✗ | — | ~ | Parse + Reject-Zero; **kein Effekt-Test** |
| `--list tasks` | ✓ | ✓ | — | — | ~ | `test_cli` ListTasks |
| `--list workflows` | ✗ | ✗ | ✗ | — | ✗ | **Fehlt komplett** |
| `--list steps` | ✗ | ✗ | ✗ | — | ✗ | **Fehlt komplett** |
| `--list phases` | ✓ | ✗ | ✗ | — | ~ | Nur Parse in `test_cli_app` |
| `-p, --phase` | ✓ | ✓ | ✓ | — | ✓ | Colon + bracket syntax in Integration + System |
| `-s, --step` | ~ | ✓ | ✓ | — | ✓ | Integration + System; kein dedizierter Unit-Parse-Test |
| `--log-file` | ✗ | ✗ | ✗ | — | ✗ | **Fehlt komplett** (Parse + Runtime) |
| `--no-log-file` | ✗ | ✗ | ✗ | — | ✗ | **Fehlt komplett** |
| `--` user options | ✓ | ✗ | ✗ | — | ~ | Parse only in `test_cli_app` |
| Flag-Konflikte (`--silent --verbose`) | ✓ | ✗ | — | — | ~ | Unit RejectsConflictingOutputFlags |

**CLI-Zusammenfassung:** 8 Flags ohne Runtime-Integration-Test · 3 Flags komplett fehlend (`--log-file`, `--no-log-file`, `--list workflows/steps`)

---

## 2. `beez.config()` / Config-Schema

### 2.1 `performance`

| Key | Unit | Integration | System | Status | Notes |
|-----|------|-------------|--------|--------|-------|
| `max_threads` | ✓ | ✗ | ✗ | ~ | `test_lua_settings`, `test_settings`; kein Runtime |
| `cache_write_strategy` | ✓ | ✗ | ✗ | ~ | `test_performance_options`, schema completions |
| `cache_fs_metadata` | ~ | ✗ | ✗ | ~ | Schema completions only |
| `use_mmap_for_hashing` | ✗ | ✗ | ✗ | ✗ | Schema definiert, **kein Test** |
| `mmap_hashing_min_bytes` | ✓ | ✗ | ✗ | ~ | `test_performance_options` NormalizesMmapThreshold |
| `optimize_gc_for_throughput` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `pin_threads_to_cores` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |

### 2.2 `cache`

| Key | Unit | Integration | System | Status | Notes |
|-----|------|-------------|--------|--------|-------|
| `path` | ✓ | ~ | ✗ | ~ | `test_settings`, `test_lua_settings`; kein System-Fixture |
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
| `icons` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `animation.progress` | ✓ | ✗ | ✗ | ~ | `test_ui_options`, `test_lua_settings` |
| `animation.indicator` | ✓ | ✗ | ✗ | ~ | `test_ui_options` |
| `animation.indicator_spin_interval` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `log_level` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `hide_cache_hits` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `prefix` / `prefix_format` | ✓ | ~ | ✗ | ~ | `test_ui_options` + Integration clean-mode worker test |
| `show_time_saved` | ✓ | ✗ | ✗ | ~ | `test_ui_options` FormatsFullyCachedRunEnd |
| `summary` | ✓ | ✗ | ✗ | ~ | `test_ui_options` |
| `logging.run_log` | ~ | ✗ | ✗ | ~ | `test_logging_settings` (run_log_writer) |
| `logging.run_log_file` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `logging.log_steps` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `logging.workers` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `logging.workers_dir` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |

### 2.4 `env`

| Key | Unit | Integration | System | Status | Notes |
|-----|------|-------------|--------|--------|-------|
| `load_dotenv` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `dotenv_overrides_system` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `files` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `vars` | ✓ | ✗ | ✗ | ~ | `test_lua_settings`, `test_settings` ApplyEnvironment |
| `hash_vars` | ~ | ✗ | ✗ | ~ | `test_env_settings` ResolveUsesDefaultsForHashLists |
| `ignore_vars_for_hashing` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |
| `mask_secrets` | ✗ | ✗ | ✗ | ✗ | **Fehlt** |

### 2.5 Config-Merge & CLI-Override

| Szenario | Unit | Integration | System | Status |
|----------|------|-------------|--------|--------|
| Global → Project (`beez.config`) merge | ✓ | ✗ | ✗ | ~ |
| CLI überschreibt Project (`--no-cache`, `--verbose`) | ✓ | ✗ | ✗ | ~ |
| `--show-config` zeigt merged Config | ✗ | ✗ | ✗ | ✗ |
| `build.lua` Config wirkt auf Cache-Pfad | ✓ | ~ | ✗ | ~ |
| `build.lua` Config wirkt auf UI-Output | ✗ | ~ | ✗ | ~ |

**Config-Zusammenfassung:** ~25 Keys ohne direkten Test · kein System-Fixture für Config-heavy Projects

---

## 3. DSL-Elemente (`build.lua`)

| Element | Unit | Integration | System | Fuzz | Status | Notes |
|---------|------|-------------|--------|------|--------|-------|
| `task(name, cmd)` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `task(name, {cmds})` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `task(name, {step refs})` | ✓ | ✓ | ✓ | — | ✓ | |
| `step({name, phase, scope, run})` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `step({description})` | ✓ | ✓ | — | — | ~ | |
| `step({run = function})` | ✓ | ✓ | — | — | ~ | spawn/wait in Integration |
| `configure_step` before/after registration | ✓ | ✓ | — | — | ✓ | |
| Step inline config | ✓ | ✓ | — | — | ✓ | |
| `artifacts` / `inputs` / `outputs` | ✓ | ✓ | — | ✓ | ✓ | Cache pipelines |
| `mutate` | ✓ | — | — | — | ~ | Unit only |
| `order` declaration | ✓ | — | — | — | ~ | Unit only |
| `workflow(name, sequential)` | ✓ | ✓ | ✓ | ✓ | ✓ | |
| `workflow(name, {parallel})` | ✗ | ✗ | ✗ | ✗ | ✗ | **Entfernt** |
| `beez.config({...})` | ✓ | ~ | ✗ | ~ | ~ | Unit + 1 Integration case |
| `beez.env(name)` | ✓ | ✗ | ✗ | ✓ | ~ | Unit + fuzz seed |
| Phase-task shorthand `{phase, scope, run}` | ~ | ✗ | ✗ | ✓ | ~ | Negative in unit; fuzz seed only |
| `depends_on` (Task) | ✗ | ✗ | ✗ | — | ✗ | **Nicht implementiert** (docs example) |
| Duplikat-Name (task/step) | ✗ | ✗ | ✗ | — | ✗ | **Fehlt** |
| Leere Task-Tabelle | ✗ | ✗ | ✗ | — | ✗ | **Fehlt** |
| Step ohne `name`/`phase`/`run` | ~ | ✗ | ✗ | — | ~ | Teilweise negative (missing run) |
| Ungültiger `run`-Typ | ✗ | ✗ | ✗ | — | ✗ | **Fehlt** |
| Syntax-Fehler | ✓ | ✓ | ✓ | ✓ | ✓ | |
| Orphan task (kein Workflow) | ✓ | ✓ | ✓ | ✓ | ✓ | |

---

## 4. Runtime / Orchestrierung

| Verhalten | Unit | Integration | System | Status |
|-----------|------|-------------|--------|--------|
| Task-Ausführung (Shell) | ✓ | ✓ | ✓ | ✓ |
| Step-by-name (`-s`) | ✓ | ✓ | ✓ | ✓ |
| Phase-Ausführung (`-p`) | ✓ | ✓ | ✓ | ✓ |
| Workflow-Ausführung | ✓ | ✓ | ✓ | ✓ |
| Parallel phases in workflow | ✗ | ✗ | ✗ | **Entfernt** |
| Worker spawn/wait | ✓ | ✓ | — | ~ |
| Step-Cache Hit/Miss | ✓ | ✓ | ✗ | ~ |
| Success-Cache Hit/Miss | ✓ | ✓ | ✗ | ~ |
| Cache disabled → kein Skip | ✓ | ✓ | ✗ | ~ |
| Dry-run skip execution | ✓ | ✓ | — | ~ |
| Artifact dependency ordering | ✓ | — | — | ~ |
| Parallel independent steps | ✓ | — | — | ~ |
| Unknown name → error + DidYouMean | ✓ | ✓ | ✓ | ✓ |
| Missing build.lua | ✓ | ✓ | ✓ | ✓ |
| Task failure → non-zero exit | ✓ | ✓ | ✓ | ✓ |
| Elapsed duration reporting | — | ✓ | — | ~ |

---

## 5. Source-Module vs. Unit-Tests

| `src/` Modul | Unit-Test-Datei | Status |
|--------------|-----------------|--------|
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
| `core/model/step_config.cpp` | indirekt via DSL/cache | ~ |
| `plugins/host/plugin_host.cpp` | `plugin/host/test_plugin_host.cpp` | ✓ |
| `core/execution/concurrency/worker_pool.cpp` | `core/execution/concurrency/test_worker_pool.cpp` | ✓ |
| `core/execution/concurrency/thread_pool.cpp` | `core/execution/concurrency/test_thread_pool.cpp` | ✓ |
| `plugins/lua/lua_dsl.cpp` | `test_lua_dsl.cpp` | ✓ |
| `plugins/lua/lua_settings.cpp` | `test_lua_settings.cpp` | ✓ |
| `plugins/lua/lua_step_config.cpp` | indirekt | ~ |
| `plugins/shell/shell_executor.cpp` | `test_shell_executor.cpp` | ✓ |
| `cli/cli_app.cpp` | `test_cli_app.cpp` | ✓ |
| `cli/run_target.cpp` | `test_run_target.cpp` | ~ |
| `cli/install_completion.cpp` | Integration only | ~ |
| `cli/list_formatter.cpp` | `test_list_formatter.cpp` | ✓ |
| `cli/name_suggestion.cpp` | `test_name_suggestion.cpp` | ✓ |
| `logging/run_log_writer.cpp` | `test_logging_settings.cpp` | ~ |
| `logging/recording_logger.cpp` | — | ✗ |
| `logging/progress_spinner.cpp` | indirekt ui_options | ~ |
| `logging/spdlog_backend.cpp` | indirekt logger | ~ |
| `logging/output_mode.cpp` | `test_output_mode.cpp` | ✓ |
| `logging/worker_output_format.cpp` | `test_worker_output_format.cpp` | ✓ |
| `app/main.cpp` | via Integration CLI | ~ |

---

## 6. System-Fixtures

| Fixture | Pfad | Abgedeckte Szenarien | Status |
|---------|------|----------------------|--------|
| `minimal` | `fixtures/minimal/` | hello/fail/clean tasks | ✓ |
| `workflows` | `fixtures/workflows/` | build/ci workflow, phases, steps | ✓ |
| `phase-tasks` | `fixtures/phase-tasks/` | Phase/step invocation | ✓ |
| `invalid-syntax` | `fixtures/invalid-syntax/` | Parse error | ✓ |
| `empty` | `fixtures/empty/` | Missing build.lua | ✓ |
| `config-cache` | — | Custom cache path/algo | ✗ geplant |
| `config-ui` | — | UI output_mode/theme | ✗ geplant |
| `config-env` | — | env.vars in steps | ✗ geplant |
| `cache-behavior` | — | Hit/miss/re-run | ✗ geplant |
| `flag-matrix` | — | CLI flag combinations | ✗ geplant |

---

## 7. Fuzzer

| Bereich | Corpus-Seeds | Dictionary | Status |
|---------|--------------|------------|--------|
| `task()` forms | `step.lua`, `task_with_step.lua`, `orphan_task.lua` | ✓ | ✓ |
| `workflow()` forms | `workflow_sequential.lua` | ✓ | ✓ |
| `step()` forms | `step.lua`, `step_config.lua`, `step_artifact_fields.lua` | ✓ | ✓ |
| `beez.env()` | `beez_env.lua` | — | ~ |
| `beez.config()` | — | — | ✗ |
| Invalid / mixed | `invalid_syntax.lua`, `mixed.lua` | — | ~ |
| Phase-task shorthand | `phase_task.lua` | — | ~ |
| Worker spawn | `worker_spawn.lua` | — | ~ |
| Nested config tables | — | — | ✗ geplant |
| Invalid config types | — | — | ✗ geplant |
| Missing required fields | — | — | ✗ geplant |
| Large/stress tables | — | — | ✗ geplant |
| Dictionary keywords | 12 Einträge | klein | ~ |

**Fuzz-Zusammenfassung:** 12 Seeds · kein Config-Fuzzing · Dictionary unvollständig

---

## 8. Priorisierte Lücken (Input für Phasen 2–5)

### P0 — Runtime-Flags ohne Integration-Test

1. `--silent`, `--error` (Output-Verhalten)
2. `--show-config` (merged Config Output)
3. `--no-cache` via CLI subprocess (nicht nur in-code)
4. `--update` (Cache storage update)
5. `--log-file`, `--no-log-file`
6. `--list workflows`, `--list steps`
7. `-j/--threads` (Effekt)

### P1 — Config aus `build.lua` ohne System-Test

1. Custom `cache.path` → Dateien landen dort
2. `ui.output_mode` → Output-Format
3. `env.vars` → Step sieht Variable
4. CLI-Override vs. Project-Config

### P2 — DSL Negative / Edge Cases

1. Duplikat-Namen
2. Ungültige Typen in Config/Step
3. Leere Tabellen
4. `depends_on` (wenn implementiert: Red-Test zuerst)

### P3 — Ungetestete Module

1. `config_paths.cpp`
2. `recording_logger.cpp`
3. Erweiterte `lua_step_config` Tests

### P4 — Fuzzer

1. ≥10 neue Corpus-Seeds (Config, invalid, stress)
2. Dictionary erweitern (config keys, DSL keywords)

---

## 9. Test-Dateien-Plan (neu)

| Datei | Ebene | Priorität |
|-------|-------|-----------|
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

## 10. Metriken (Baseline)

| Metrik | Wert |
|--------|------|
| Unit-Test-Dateien | 41 |
| Integration-Test-Dateien | 8 (+ 3 shell scripts) |
| System-Szenario-Dateien | 4 |
| System-Fixtures | 5 |
| Fuzz-Corpus-Seeds | 12 |
| CLI-Flags gesamt | 22 |
| CLI-Flags mit Runtime-Integration-Test | ~10 |
| Config-Schema-Keys (geschätzt) | ~45 |
| Config-Keys mit Unit-Test | ~20 |
| DSL-Elemente | 18 |
| `src/` Module ohne direkten Unit-Test | 4 (`config_paths`, `recording_logger`, `progress_spinner`, `spdlog_backend`) |

---

*Aktualisieren nach jeder Test-Phase. Siehe auch [`vertical-feature.md`](vertical-feature.md) und [`../Beez.wiki/Testing.md`](https://github.com/Coditary/Beez/wiki/Testing).*
